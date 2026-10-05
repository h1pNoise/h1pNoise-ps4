/* GoldHEN BinLoader entry. This payload returns to its caller; it never exits
   or kills the hosting process. No firmware-specific kernel/text patches.
   Module ABI reference: flatz's dynlib example and DPI Payload/lib/dl.c. */
#include "vendor/libjbc/jailbreak.h"
#include "installer_access.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <orbis/UserService.h>
extern int pkg_update_payload_main(void);
extern int payload_bind_symbols(void);
int payload_errno;
int *__error(void){return &payload_errno;}
int *__errno_location(void){return &payload_errno;}
#ifndef HARBOR_PAYLOAD_RUNTIME_TEST
asm(".section .text.entry,\"ax\",@progbits\n.global _start\n_start:\n"
    "lea __bss_start(%rip),%rdi\nlea __bss_end(%rip),%rcx\nsub %rdi,%rcx\nxor %eax,%eax\ncld\nrep stosb\njmp payload_main\n.text\n");
/* Raw syscall errors are local to our wrappers, never overwrite host errno. */
#define RAW(name,num) asm(".global " #name "\n" #name ":\nmov $" #num ",%rax\nmov %rcx,%r10\nsyscall\njnc 1f\nmov %eax,payload_errno(%rip)\nmov $-1,%rax\n1: ret\n");
RAW(payload_open,5) RAW(close,6) RAW(read,3) RAW(write,4)
RAW(getpid,20) RAW(geteuid,25) RAW(kill,37) RAW(ioctl,54)
RAW(fsync,95) RAW(payload_rename,128) RAW(socketpair,135)
RAW(payload_access,33) RAW(payload_unlink,10) RAW(fchmod,124)
RAW(ftruncate,480) RAW(payload_load,594) RAW(payload_info,608)
RAW(payload_sym,591)
#endif
extern int payload_open(const char*,int,...),payload_access(const char*,int);
extern int payload_rename(const char*,const char*),payload_unlink(const char*);
long payload_load(const char*,int,int*,int);
long payload_info(int,int,void*);
long payload_sym(int,const char*,void**);
void *memcpy(void *d,const void *s,size_t n){unsigned char *a=d;const unsigned char *b=s;while(n--)*a++=*b++;return d;}
void *memset(void *d,int c,size_t n){unsigned char *a=d;while(n--)*a++=(unsigned char)c;return d;}
/* These are used before any native libc has been loaded. */
size_t strlen(const char *s){size_t n=0;while(s[n])n++;return n;}
struct Segment{uint64_t addr;uint32_t size,flags;};
struct ModuleInfo{size_t size;char name[256];int id;uint32_t tls_index;uint64_t tls_addr;uint32_t tls_init_size,tls_size,tls_offset,tls_align;uint64_t init,fini,reserved1,reserved2,eh_hdr,eh;uint32_t eh_hdr_size,eh_size;struct Segment segment[4];uint32_t count,refs;};
int payload_module(const char *path){
 int handle=0;if(payload_load(path,0,&handle,0)||handle<=0)return -1;
 struct ModuleInfo info;memset(&info,0,sizeof(info));info.size=sizeof(info);
 if(payload_info(handle,0,&info))return -1;
 if(info.refs<2&&info.init){int rc=((int(*)(size_t,void*,void*))(uintptr_t)info.init)(0,NULL,NULL);if(rc)return -1;}
 return handle;
}
int payload_resolve(int handle,const char *name,void **out){*out=NULL;return payload_sym(handle,name,out)||!*out?-1:0;}
/* Resolve our private /data paths against the real disk after the sandbox
   escape. Only our calls are translated; the host's routines are untouched. */
static const char *physical(const char *s,char out[800]){
 if(s&&s[0]=='/'&&s[1]=='d'&&s[2]=='a'&&s[3]=='t'&&s[4]=='a'&&(s[5]=='/'||s[5]==0)){
  size_t n=strlen(s);if(n+6>=800)return NULL;memcpy(out,"/user",5);memcpy(out+5,s,n+1);return out;
 }return s;
}
void *payload_fopen_ptr;
FILE *fopen(const char *p,const char *mode){char out[800];p=physical(p,out);return p?((FILE*(*)(const char*,const char*))payload_fopen_ptr)(p,mode):NULL;}
int open(const char *p,int flags,...){char out[800];p=physical(p,out);return p?payload_open(p,flags,0600):-1;}
int access(const char *p,int mode){char out[800];p=physical(p,out);return p?payload_access(p,mode):-1;}
int remove(const char *p){char out[800];p=physical(p,out);return p?payload_unlink(p):-1;}
int rename(const char *a,const char *b){char x[800],y[800];a=physical(a,x);b=physical(b,y);return a&&b?payload_rename(a,b):-1;}
/* The payload has saved, elevated credentials for its entire bounded
   transaction, restored by payload_main even when initialization fails. */
int installer_with_access(int (*f)(char*,size_t),char *e,size_t c){return f(e,c);}
int installer_with_permissions(int (*f)(void*,char*,size_t),void *p,char *e,size_t c){return f(p,e,c);}
int ps4_active_user(int32_t *user,char *error,size_t cap){
 OrbisUserServiceInitializeParams p={.priority=ORBIS_KERNEL_PRIO_FIFO_NORMAL};
 int rc=sceUserServiceInitialize(&p);if(rc&&(uint32_t)rc!=ORBIS_USER_SERVICE_ERROR_ALREADY_INITIALIZED)goto failed;
 rc=sceUserServiceGetForegroundUser(user);if(rc||*user<0)goto failed;return 0;
failed:snprintf(error,cap,"Seleciona um utilizador na PS4 (0x%08X).",(unsigned)rc);return -1;
}
int payload_main(void){
 struct jbc_cred saved,elevated;
 if(jbc_get_cred(&saved))return -1;
 elevated=saved;if(jbc_jailbreak_cred(&elevated))return -1;
 elevated.jdir=0;elevated.sceProcType=UINT64_C(0x3800000000000010);elevated.sonyCred|=UINT64_C(1)<<62;
 if(jbc_set_cred(&elevated)){jbc_set_cred(&saved);return -1;}
 int rc=payload_bind_symbols();
 if(rc){
  int fd=payload_open("/user/data/pkg/update-install-debug.log",0x209,0600);
  if(fd>=0){const char *message="pkg-payload-test falha ao resolver modulos/API; app deve permanecer aberta\n";write(fd,message,strlen(message));close(fd);}
 }else rc=pkg_update_payload_main();
 int restored=jbc_set_cred(&saved);return restored?-1:rc;
}
