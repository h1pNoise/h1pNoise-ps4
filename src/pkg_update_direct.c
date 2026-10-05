#include "app.h"
#include "pkg_update.h"
#include "update_config.h"
#include "version.h"
#include "installer_access.h"
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>
#include <orbis/AppInstUtil.h>

#define ATTEMPT "/data/harbor/pkg-direct-attempt.h1p"
extern void link_stage(const char *,int);
static UpdateManifest candidate;
static char install_path[740];
static void report(const char *stage,int rc){
 FILE *f=fopen("/data/pkg/update-install-debug.log","a");
 if(f){fprintf(f,"pkg-direct-test %s 0x%08X\n",stage,(unsigned)rc);fflush(f);fsync(fileno(f));fclose(f);}
 link_stage(stage,rc);
}
static uint32_t le32(const unsigned char *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned le16(const unsigned char *p){return p[0]|p[1]<<8;}
static int sfo_field(const unsigned char *p,size_t n,const char *key,char *out,size_t cap){
 if(n<20||le32(p)!=0x46535000)return -1;
 uint32_t count=le32(p+16),keys=le32(p+8),values=le32(p+12);if(count>(n-20)/16||keys>=n||values>=n)return -1;
 int found=0;for(uint32_t i=0;i<count;i++){
  const unsigned char *e=p+20+16*i;uint64_t k=(uint64_t)keys+le16(e),v=(uint64_t)values+le32(e+12);size_t len=le32(e+4);
  if(k>=n||v>=n||len>n-v||!memchr(p+k,0,n-k))return -1;
  if(!strcmp((char*)p+k,key)){if(found++||le16(e+2)!=0x204||!len||len>cap||p[v+len-1]||memchr(p+v,0,len-1))return -1;memcpy(out,p+v,len);}
 }return found==1?0:-1;
}
static uint32_t direct_be32(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static char installed_version[6];
static int installed_read(char *error,size_t cap){
 /* Read the actual installed PKG, not potentially stale appmeta. A patch or
    inaccessible external installation is unsupported in this experiment. */
 if(!access("/user/patch/HBRW00001/patch.pkg",F_OK)){snprintf(error,cap,"A app tem um patch instalado. Este teste nao o substitui.");return -1;}
 FILE *f=fopen("/user/app/HBRW00001/app.pkg","rb");if(!f)goto failed;
 if(fseeko(f,0,SEEK_END)){fclose(f);goto failed;}int64_t size=ftello(f);
 if(size<8192||fseeko(f,0,SEEK_SET)){fclose(f);goto failed;}
 unsigned char *p=malloc(512*1024);if(!p){fclose(f);goto failed;}
 size_t n=fread(p,1,512*1024,f);int bad=ferror(f);fclose(f);int valid=0;
 if(!bad&&n>=8192){uint32_t entries=direct_be32(p+0x10),table=direct_be32(p+0x18);
  if(table<=n&&entries<=(n-table)/32)for(uint32_t i=0;i<entries;i++){
   const unsigned char *e=p+table+32*i;if(direct_be32(e)!=0x1000)continue;
   uint32_t off=direct_be32(e+16),length=direct_be32(e+20);if(off>n||length>n-off)break;
   UpdateManifest current={.size=(uint64_t)size};
   if(!sfo_field(p+off,length,"APP_VER",current.sfo,sizeof(current.sfo))&&strlen(current.sfo)==5&&current.sfo[2]=='.'&&current.sfo[0]>='0'&&current.sfo[0]<='9'&&current.sfo[1]>='0'&&current.sfo[1]<='9'&&current.sfo[3]>='0'&&current.sfo[3]<='9'&&current.sfo[4]>='0'&&current.sfo[4]<='9'&&!update_pkg_metadata(p,n,(uint64_t)size,&current)){memcpy(installed_version,current.sfo,6);valid=1;}
  }
 }
 free(p);if(valid)return 0;
failed:
 snprintf(error,cap,"Nao foi possivel confirmar a versao atualmente instalada. Nada foi instalado.");return -1;
}
static int modules(char *error,size_t cap){
 static int loaded; if(loaded)return 0;
 int rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_APP_INST_UTIL);
 if(rc<0)rc=(int32_t)sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx",0,NULL,0,NULL,NULL);
 report("carregar AppInstUtil direto",rc);
 if(rc<0){snprintf(error,cap,"AppInstUtil indisponivel (0x%08X). Envia update-install-debug.log.",(unsigned)rc);return -1;}
 loaded=1;return 0;
}
static int installed_matches(char *error,size_t cap){
 return update_file_verify("/user/app/HBRW00001/app.pkg",&candidate,error,cap);
}
static int native_install(void *unused,char *error,size_t cap){
 (void)unused;
 /* Initialize in the same temporary ShellCore authorization as installation.
    Access to the module file alone does not authorize installer IPC. */
 int rc=sceAppInstUtilInitialize();report("inicializar AppInstUtil com permissoes de instalacao",rc);
 if(rc){snprintf(error,cap,"AppInstUtil recusou a inicializacao (0x%08X). Envia update-install-debug.log.",(unsigned)rc);return -1;}
 report("uid efetivo antes de instalar",(int)geteuid());
 /* Diagnostic only: a sandboxed caller can see /data while ShellCore needs
    the physical /user/data path. Do not equate caller access with IPC access. */
 int fd=open(install_path,O_RDONLY);report("abrir caminho fisico da copia",fd<0?-errno:0);if(fd>=0)close(fd);
 report("antes AppInstallPkg; app continua aberta",0);
 rc=sceAppInstUtilAppInstallPkg(install_path,NULL);
 report("resultado AppInstallPkg direto",rc);
 if(rc){snprintf(error,cap,"Instalacao direta recusada (0x%08X). O PKG original continua em /data/pkg. Envia update-install-debug.log.",(unsigned)rc);return -1;}
 return 0;
}
/* A direct install moves this inode into /user/app. A normal sequential copy
   can have the right hash yet lack the disk allocation needed by PFS on launch.
   Prepare only our new, empty descriptor; never touch the installed app file.
   ABI and commands: https://flatz.github.io/ (PKG installation from HDD).
   Unsupported firmware/device/commands must stop before installation. */
static int prepare_copy(void *context,char *error,size_t cap){
 int fd=*(int*)context,rc=ftruncate(fd,0);
 report("truncar copia nova antes da reserva",rc);if(rc)goto failed;
 int device=open("/dev/gsched_is.ctl",O_RDONLY);
 if(device<0){rc=-errno;report("abrir dispositivo de reserva",rc);goto failed;}
 struct {void *file;uint32_t slot,priority;unsigned char reserved[16];} schedule={0};
 _Static_assert(sizeof(schedule)==32,"gsched ioctl buffer size");
 schedule.file=(void*)(uintptr_t)fd;schedule.slot=1;schedule.priority=7;
 rc=ioctl(device,0xC0209406UL,&schedule);if(rc==-1)rc=-errno;
 report("definir slot e prioridade da copia",rc);close(device);if(rc)goto failed;
 struct {uint64_t size,zero,flags,alignment;} allocation={candidate.size,0,0x80,0};
 _Static_assert(sizeof(allocation)==32,"FFS allocation ioctl ABI");
 rc=ioctl(fd,0xC02066A1UL,&allocation);if(rc==-1)rc=-errno;
 report("reservar blocos da copia antes de escrever",rc);if(rc)goto failed;
 return 0;
failed:
 snprintf(error,cap,"Nao foi possivel preparar o PKG no disco (0x%08X). Nada foi instalado. O original ficou em /data/pkg; envia update-install-debug.log.",(unsigned)rc);return -1;
}
/* AppInstallPkg may move its source. Install a verified private copy so that
   a refusal/crash cannot consume the user's original downloaded update. */
static int copy_pkg(const char *from,const char *to,char *error,size_t cap){
 FILE *in=fopen(from,"rb");if(!in)goto failed;
 int fd=open(to,O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0){fclose(in);goto failed;}
 if(installer_with_permissions(prepare_copy,&fd,error,cap)){fclose(in);close(fd);remove(to);return -1;}
 unsigned char *buffer=malloc(65536);int bad=!buffer;uint64_t total=0;
 while(!bad){size_t n=fread(buffer,1,65536,in);if(!n){bad=ferror(in);break;}if(total+n>candidate.size){bad=1;break;}
  size_t at=0;while(at<n){ssize_t k=write(fd,buffer+at,n-at);if(k<=0){bad=1;break;}at+=(size_t)k;}total+=n;
 }
 free(buffer);fclose(in);
 /* Only this verified installer copy is shared read-only with the system
    service. Set mode explicitly after writing, even with a restrictive umask.
    The durable attempt remains private (0600). */
 if(!bad&&total==candidate.size){int rc=fchmod(fd,0644);report("permissoes da copia 0644",rc);if(rc)bad=1;}
 if(fsync(fd))bad=1;if(close(fd))bad=1;
 if(bad||total!=candidate.size){remove(to);goto failed;}
 if(update_file_verify(to,&candidate,error,cap)){remove(to);return -1;}return 0;
failed:snprintf(error,cap,"Nao foi possivel criar a copia de instalacao. O PKG original foi preservado.");return -1;
}
int pkg_update_direct(const char *path,char *error,size_t cap){
 unsigned char raw[UPDATE_MANIFEST_MAX+1];size_t n;
 lock(&app.mu);n=app.update.signed_size;if(n<=UPDATE_MANIFEST_MAX)memcpy(raw,app.update.signed_manifest,n);unlock(&app.mu);
 if(!n||n>UPDATE_MANIFEST_MAX||update_manifest_read(raw,n,UPDATE_PUBLIC_KEY,&candidate,error,cap))return -1;
 char expected[700],copy[720];snprintf(expected,sizeof(expected),"/data/pkg/h1pNoise-update-%u.pkg",candidate.build);
 if(strcmp(expected,path)||candidate.build<=APP_BUILD||strcmp(candidate.sfo,APP_SFO_VERSION)<=0){snprintf(error,cap,"Atualizacao ou destino invalido.");return -1;}
 if(update_file_verify(path,&candidate,error,cap))return -1;
 mkdir("/data/harbor",0777);
 /* A saved attempt prevents retry after an accepted request or a crash. A
    newer running binary can retire the previous signed attempt on next use. */
 FILE *previous=fopen(ATTEMPT,"rb");if(previous){
  unsigned char old[UPDATE_MANIFEST_MAX+1];size_t used=fread(old,1,sizeof(old),previous);int bad=ferror(previous);fclose(previous);UpdateManifest m;
  if(bad||used>UPDATE_MANIFEST_MAX||update_manifest_read(old,used,UPDATE_PUBLIC_KEY,&m,error,cap)||m.build>APP_BUILD){snprintf(error,cap,"Ja houve uma tentativa de instalacao. Confirma a versao instalada e envia update-install-debug.log antes de repetir.");return -1;}
  if(remove(ATTEMPT)){snprintf(error,cap,"Nao foi possivel concluir o pedido anterior.");return -1;}
 }
 if(installer_with_access(modules,error,cap))return -1;
 /* Read the actual installed identity/version; refuse patches and older PKGs. */
 int installed_rc=installer_with_access(installed_read,error,cap);report("ler versao do PKG instalado",installed_rc);if(installed_rc)return -1;
 if(strcmp(candidate.sfo,installed_version)<=0){snprintf(error,cap,"Esta versao ou uma versao mais recente ja esta instalada. Volta a abrir a app para usar a nova versao.");return -1;}
 char detail[512]={0};
 if(!installer_with_access(installed_matches,detail,sizeof(detail))){snprintf(error,cap,"Esta atualizacao ja esta instalada. A versao em execucao muda quando voltares a abrir a app.");return -1;}
 snprintf(copy,sizeof(copy),"%s.install.pkg",path);
 if(storage_check(app.root,candidate.size+64*1024*1024ULL,error,cap)||copy_pkg(path,copy,error,cap))return -1;
 int fd=open(ATTEMPT,O_WRONLY|O_CREAT|O_EXCL,0600);
 if(fd<0){remove(copy);snprintf(error,cap,"Nao foi possivel guardar o pedido de instalacao.");return -1;}
 size_t at=0;int bad=0;while(at<n){ssize_t k=write(fd,raw+at,n-at);if(k<=0){bad=1;break;}at+=(size_t)k;}
 if(fsync(fd))bad=1;if(close(fd))bad=1;
 if(bad){remove(ATTEMPT);remove(copy);snprintf(error,cap,"Nao foi possivel guardar o pedido de instalacao.");return -1;}
 lock(&app.mu);app.update.install_sent=1;unlock(&app.mu);
 snprintf(install_path,sizeof(install_path),"/user%s",copy);
 if(installer_with_permissions(native_install,NULL,error,cap))return -1;
 for(unsigned i=0;i<60;i++){
  if(!installer_with_access(installed_matches,detail,sizeof(detail))){report("PKG instalado confirmado; sem fechar app",0);return 0;}
  sleep_ms(500);
 }
 report("pedido aceite; instalacao ainda nao confirmada",-1);
 snprintf(error,cap,"O sistema aceitou o PKG, mas ainda nao foi possivel confirmar a instalacao. Nao repitas o pedido; envia update-install-debug.log. O PKG original foi preservado.");return -1;
}
