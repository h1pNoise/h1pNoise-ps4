#include "pkg_update.h"
#include <stdint.h>
#include <stddef.h>
extern int payload_open(const char *,int,...),close(int),fsync(int);
extern long write(int,const void *,size_t);
/* Available before libc, credentials or module initialization. Logging is
   best-effort; it never grants permission or confirms the install handoff. */
static void save(const char *local,const char *physical,const char *data,size_t n,int flags){
 int fd=payload_open(local,flags,0600);if(fd<0)fd=payload_open(physical,flags,0600);
 if(fd<0)return;
 size_t at=0;for(unsigned tries=0;at<n&&tries<8;tries++){
  long used=write(fd,data+at,n-at);if(used<=0||(size_t)used>n-at)break;at+=(size_t)used;
 }
 fsync(fd);close(fd);
}
void payload_progress(const char *stage,int code){
 char line[192];size_t n=0;const char *prefix="pkg-payload-runtime ";
 while(*prefix)line[n++]=*prefix++;
 for(size_t i=0;stage[i]&&i<128;i++)line[n++]=stage[i];
 line[n++]=' ';line[n++]='0';line[n++]='x';
 const char hex[]="0123456789ABCDEF";
 for(int shift=28;shift>=0;shift-=4)line[n++]=hex[((uint32_t)code>>shift)&15];
 line[n++]='\n';
 save("/data/pkg/update-install-debug.log","/user/data/pkg/update-install-debug.log",line,n,0x209);
 save(PKG_UPDATE_ROOT "/runtime-status","/user" PKG_UPDATE_ROOT "/runtime-status",line,n,0x601);
}
