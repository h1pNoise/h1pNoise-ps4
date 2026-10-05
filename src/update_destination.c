#include "app.h"
#include "version.h"
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
#include "installer_access.h"
#include "vendor/libjbc/jailbreak.h"
#include <sys/syscall.h>
/* The SDK's renameat/unlinkat return ENOSYS on PS4. Use the kernel ABI,
   relative to an opened USB directory, after restoring the app's roots. */
#ifdef HARBOR_USB_TEST
extern int update_usb_native(int,int,const char *,int,const char *);
#else
static int update_usb_native(int number,int fd,const char *name,int value,const char *other){
 long result;unsigned char failed;register long fourth __asm__("r10")=(long)other;
 __asm__ volatile("syscall; setc %1":"=a"(result),"=qm"(failed):"a"((long)number),"D"((long)fd),"S"(name),"d"((long)value),"r"(fourth):"rcx","r11","memory");
 if(failed){errno=(int)result;return -1;}return (int)result;
}
#endif
static int usb_access_failed;
static void usb_diagnostic(const char *step,int direct,int elevated,int restored){
#ifndef HARBOR_USB_TEST
 FILE *log=fopen("/data/pkg/usb-debug.log","a");
 if(log){fprintf(log,"%s usb_step=%s direct=%d elevated=%d restore=%d\n",APP_VERSION,step,direct,elevated,restored);fclose(log);}
#else
 (void)step;(void)direct;(void)elevated;(void)restored;
#endif
}
static int usb_retryable(int error){return error==EPERM||error==EACCES||error==ENOENT||error==ENOSYS;}
static int usb_operation(int number,int directory,const char *name,int value,const char *other,const char *path,const char *to){
 if(usb_access_failed){errno=EIO;return -1;}
 int rc=update_usb_native(number,directory,name,value,other);
 if(rc>=0)return rc;
 int direct=errno,activated=-999,restored=-999,operation_error=direct;
 /* An external directory descriptor is readable without necessarily granting
    creation/rename rights. Retry permission denial on that same descriptor.
    First keep cdir/rdir/jdir unchanged, as for the disk-space query. */
 if(usb_retryable(direct)){
  if(installer_credentials_trylock()){errno=EBUSY;usb_diagnostic("credentials-busy",direct,-999,-999);return -1;}
  struct jbc_cred saved,elevated;
  if(!jbc_get_cred(&saved)){
   elevated=saved;
   if(!jbc_jailbreak_cred(&elevated)){
    struct jbc_cred permissions=elevated;
    permissions.cdir=saved.cdir;permissions.rdir=saved.rdir;permissions.jdir=saved.jdir;
    activated=jbc_set_cred(&permissions);
    if(!activated){rc=update_usb_native(number,directory,name,value,other);operation_error=rc<0?errno:0;}
    /* Some firmware refuses directory-relative access outside the app root,
       or does not expose this syscall. The known USB path can then be opened
       through libc under a short full-root scope. Never hold it for the stream. */
    if(!activated&&rc<0&&usb_retryable(operation_error)){
     activated=jbc_set_cred(&elevated);
     if(!activated){
      rc=number==SYS_openat?open(path,value,(mode_t)(uintptr_t)other):number==SYS_renameat?rename(path,to):unlink(path);
      operation_error=rc<0?errno:0;
     }
    }
    restored=jbc_set_cred(&saved);
    if(activated||restored){if(rc>=0&&number==SYS_openat)close(rc);rc=-1;operation_error=EIO;}
    if(restored)usb_access_failed=1;
   }
  }
  installer_credentials_unlock();
 }
 usb_diagnostic(number==SYS_openat?"open-file":number==SYS_renameat?"rename-file":"remove-part",direct,rc>=0?0:operation_error,restored);
 errno=operation_error;return rc;
}
static int usb_directory(int id,char *error,size_t cap){
 if(usb_access_failed){snprintf(error,cap,"Fecha e volta a abrir a app para restaurar o acesso USB.");return -1;}
 const char *root=update_destination_root(id);int fd=open(root,O_RDONLY|O_DIRECTORY);
 if(fd>=0)return fd;
 int direct_error=errno,rc=-1,activated=-999,restored=-999;
 struct jbc_cred saved,elevated;
 if(installer_credentials_trylock()){snprintf(error,cap,"O acesso ao disco esta ocupado. Tenta novamente.");return -1;}
 if(jbc_get_cred(&saved)||((elevated=saved),jbc_jailbreak_cred(&elevated))){
  snprintf(error,cap,"Nao foi possivel obter acesso a %s (erro %d). Confirma o GoldHEN e a pen.",root,direct_error);
 }else{
  activated=jbc_set_cred(&elevated);
  if(!activated){fd=open(root,O_RDONLY|O_DIRECTORY);rc=fd>=0?0:errno;}
  /* Activation can fail partially. Restore before any network/file stream. */
  restored=jbc_set_cred(&saved);
  if(restored){usb_access_failed=1;if(fd>=0)close(fd);fd=-1;snprintf(error,cap,"Falha ao restaurar o acesso. Fecha e volta a abrir a app.");}
  else if(activated||fd<0){fd=-1;snprintf(error,cap,"A pen USB em %s nao esta acessivel (erro %d). Confirma exFAT/FAT32 e volta a ligar a pen.",root,rc);}
 }
 installer_credentials_unlock();
#ifndef HARBOR_USB_TEST
 FILE *log=fopen("/data/pkg/usb-debug.log","a");
 if(log){fprintf(log,"usb=%s direct=%d activate=%d open=%d restore=%d\n",root,direct_error,activated,rc,restored);fclose(log);}
#endif
 return fd;
}
/* Only deterministic update filenames are accepted for directory-relative I/O. */
static const char *usb_name(const char *path){
 if(strncmp(path,"/mnt/usb0/",10)&&strncmp(path,"/mnt/usb1/",10))return NULL;
 const char *name=strrchr(path,'/');if(!name||name!=path+9)return NULL;name++;
 if(strncmp(name,"h1pNoise-update-",16))return NULL;
 const char *p=name+16;if(*p<'0'||*p>'9')return NULL;while(*p>='0'&&*p<='9')p++;
 return !strcmp(p,".pkg")||!strcmp(p,".pkg.part")?name:NULL;
}
#endif
int update_destination_id(const char *id){
 if(!strcmp(id,"internal"))return 0;
 if(!strcmp(id,"usb0"))return 1;
 if(!strcmp(id,"usb1"))return 2;
 return -1;
}
const char *update_destination_root(int id){
 return id==0?app.root:id==1?"/mnt/usb0":id==2?"/mnt/usb1":NULL;
}
int update_destination_available(int id){
 const char *root=update_destination_root(id);if(!root)return 0;
 if(id==0)return 1;
#ifdef _WIN32
 /* USB destinations here belong to a PS4, not to the Windows test host. */
 return 0;
#else
 /* Opening a directory avoids depending on the SDK's struct stat ABI. */
 int fd=open(root,O_RDONLY|O_DIRECTORY);if(fd<0)return 0;close(fd);return 1;
#endif
}
int update_destination_prepare(int id,int *directory,char *error,size_t cap){
 *directory=-1;
 const char *root=update_destination_root(id);
 if(!root){snprintf(error,cap,"Destino de atualizacao invalido.");return -1;}
 if(id){
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
  *directory=usb_directory(id,error,cap);if(*directory<0)return -1;
#else
  snprintf(error,cap,"Guardar numa pen USB requer uma PS4 real.");return -1;
#endif
 }
 if(!id)make_dir(root);
 return 0;
}
FILE *update_destination_file(int directory,const char *path,const char *mode){
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 if(directory>=0){
  const char *name=usb_name(path);if(!name){errno=EINVAL;return NULL;}
  if(strcmp(mode,"wb")&&strcmp(mode,"rb")){errno=EINVAL;return NULL;}
  int flags=(!strcmp(mode,"wb")?O_WRONLY|O_CREAT|O_TRUNC:O_RDONLY)|O_NOFOLLOW;
  int fd=usb_operation(SYS_openat,directory,name,flags,(const char *)(uintptr_t)0666,path,NULL);
  if(fd<0)return NULL;FILE *f=fdopen(fd,mode);if(!f){int reason=errno;close(fd);usb_diagnostic("fdopen",reason,-999,-999);errno=reason;}return f;
 }
#else
 (void)directory;
#endif
 return fopen(path,mode);
}
int update_destination_remove(int directory,const char *path){
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 if(directory>=0){const char *name=usb_name(path);return name?usb_operation(SYS_unlinkat,directory,name,0,NULL,path,NULL):-1;}
#else
 (void)directory;
#endif
 return remove(path);
}
int update_destination_rename(int directory,const char *from,const char *to){
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 if(directory>=0){const char *a=usb_name(from),*b=usb_name(to);return a&&b?usb_operation(SYS_renameat,directory,a,directory,b,from,to):-1;}
#else
 (void)directory;
#endif
 return rename(from,to);
}
