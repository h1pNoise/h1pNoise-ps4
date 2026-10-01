#include "storage.h"
#include "version.h"
#include <sys/stat.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifndef HARBOR_SHADPS4
#include "installer_access.h"
#include "vendor/libjbc/jailbreak.h"
#endif

/* FreeBSD 9 / PS4 statfs prefix; the mount-name tail differs between SDKs.
   Reference: ps4dev/ps4sdk include/sys/mount.h and FreeBSD releng/9.0. */
typedef struct {
 uint32_t version,type;uint64_t flags,block_size,io_size,blocks,free_blocks;
 int64_t available_blocks;
} FsPrefix;
_Static_assert(offsetof(FsPrefix,block_size)==16,"PS4 statfs block size ABI");
_Static_assert(offsetof(FsPrefix,available_blocks)==48,"PS4 statfs available ABI");
typedef union {FsPrefix info;unsigned char room[4096];} FsBuffer;
typedef struct {int saved,resolved,activated,queried,restored;} AccessResult;
#ifndef HARBOR_SHADPS4
static int access_restore_failed;
#endif

#if defined(HARBOR_STORAGE_TEST)
extern int ps4_storage_native_query(int fd,void *buffer);
#elif !defined(HARBOR_SHADPS4)
static int ps4_storage_native_query(int fd,void *buffer){
 /* OpenOrbis libc delegates to the _fstatfs export. Request the modern
    kernel ABI directly (397), rather than the FreeBSD4 compatibility call
    (158). No filesystem roots, privileges or kernel memory are changed.
    PS4 HEN must permit homebrew syscalls; failure remains unknown capacity. */
 long result;unsigned char failed;
 __asm__ volatile("syscall; setc %1":"=a"(result),"=qm"(failed):
   "a"(397L),"D"((long)fd),"S"(buffer):"rcx","r11","memory");
 return failed?-(int)result:(int)result;
}
#endif

#ifndef HARBOR_SHADPS4
static int permitted_query(int fd,FsBuffer *buffer,AccessResult *access){
 struct jbc_cred saved,elevated;
 access->saved=jbc_get_cred(&saved);if(access->saved)return -1;
 elevated=saved;
 access->resolved=jbc_jailbreak_cred(&elevated);if(access->resolved)return -1;
 /* Only credentials/prison change. Preserve every directory vnode, so
    concurrent app I/O keeps the same filesystem paths and destination. */
 elevated.cdir=saved.cdir;elevated.rdir=saved.rdir;elevated.jdir=saved.jdir;
 access->activated=jbc_set_cred(&elevated);
 if(!access->activated){memset(buffer,0,sizeof(*buffer));access->queried=ps4_storage_native_query(fd,buffer);}
 /* An activation failure can be partial; always attempt exact restoration. */
 access->restored=jbc_set_cred(&saved);
 if(access->restored)access_restore_failed=1;
 if(access->activated||access->restored)return -1;
 return access->queried;
}
#endif

static int decode(const FsBuffer *buffer,uint64_t *available){
 const FsPrefix *v=&buffer->info;
 *available=0;
 if(v->version!=0x20030518u||v->free_blocks>v->blocks||
    (v->available_blocks>0&&(uint64_t)v->available_blocks>v->free_blocks))return -1;
 return storage_from_blocks(v->block_size,v->blocks,v->available_blocks,available);
}
static void diagnostic(const char *path,int opened,int native_rc,int library_rc,const FsBuffer *buffer,int result,const AccessResult *access){
#ifndef HARBOR_STORAGE_TEST
 /* One record per application run, without URL/token or changing app state. */
 static int written;
 int already=__atomic_exchange_n(&written,1,__ATOMIC_RELAXED);
 if(already&&(access->restored==-999||access->restored==0))return;
 FILE *f=fopen("/data/pkg/storage-debug.log","a");
 if(f){const FsPrefix *v=&buffer->info;
  fprintf(f,"%s path=%s open=%d native=%d library=%d version=0x%08X block=%llu total=%llu free=%llu available=%lld result=%d access_save=%d access_resolve=%d access_activate=%d access_query=%d access_restore=%d\n",
   APP_VERSION,path,opened,native_rc,library_rc,v->version,
   (unsigned long long)v->block_size,(unsigned long long)v->blocks,
   (unsigned long long)v->free_blocks,(long long)v->available_blocks,result,
   access->saved,access->resolved,access->activated,access->queried,access->restored);
  fflush(f);fsync(fileno(f));fclose(f);
 }
#else
 (void)path;(void)opened;(void)native_rc;(void)library_rc;(void)buffer;(void)result;(void)access;
#endif
}
static int query(const char *path,uint64_t *available){
 FsBuffer buffer={0};*available=0;int native_rc=-1,library_rc=-1,result=-1;
 AccessResult access={-999,-999,-999,-999,-999};
 int fd=open(path,O_RDONLY);
 if(fd<0){diagnostic(path,-errno,native_rc,library_rc,&buffer,result,&access);return -1;}
#ifndef HARBOR_SHADPS4
 native_rc=ps4_storage_native_query(fd,&buffer);
 if(!native_rc)result=decode(&buffer,available);
#endif
 if(result){
  memset(&buffer,0,sizeof(buffer));errno=0;
  library_rc=fstatfs(fd,(struct statfs *)&buffer);
  if(library_rc)library_rc=errno?-errno:library_rc;
  else result=decode(&buffer,available);
 }
#ifndef HARBOR_SHADPS4
 /* Retry permission denial only, on the already-open destination descriptor.
    A failed/unsupported syscall or invalid result is not a reason to elevate. */
 if(result&&(native_rc==-EPERM||library_rc==-EPERM)){
  if(!permitted_query(fd,&buffer,&access))result=decode(&buffer,available);
 }
#endif
 close(fd);diagnostic(path,0,native_rc,library_rc,&buffer,result,&access);return result;
}
static int free_bytes_locked(const char *path,uint64_t *available){
 if(!query(path,available))return 0;
#ifndef HARBOR_SHADPS4
 if(access_restore_failed){*available=0;return -1;}
#endif
 /* Never report a different partition as the destination's free space. */
 if(!strcmp(path,"/data/pkg")){
  struct stat destination,parent;
  if(!stat(path,&destination)&&!stat("/data",&parent)&&destination.st_dev==parent.st_dev)
   return query("/data",available);
 }
 *available=0;return -1;
}
int free_bytes(const char *path,uint64_t *available){
#ifndef HARBOR_SHADPS4
 *available=0;
 /* Do not race an installer's root/auth switch or wait with app.mu held. */
 if(installer_credentials_trylock())return -1;
 if(access_restore_failed){installer_credentials_unlock();return -1;}
 int rc=free_bytes_locked(path,available);installer_credentials_unlock();return rc;
#else
 return free_bytes_locked(path,available);
#endif
}
