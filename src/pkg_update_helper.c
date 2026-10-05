#include "pkg_update.h"
#include "update_config.h"
#include "installer_access.h"
#include "ps4_user.h"
#include <stdbool.h>
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>
#include <orbis/AppInstUtil.h>
#include <orbis/_types/sys_service.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
extern int32_t sceSystemServiceLaunchApp(const char *,const char **,LncAppParam *);
static FILE *log_file;
#ifdef HARBOR_PKG_PAYLOAD_TEST
extern void payload_progress(const char *,int);
extern const char *payload_data_path(const char *,char *,size_t);
#endif
void link_stage(const char *stage,int result){
#ifdef HARBOR_PKG_PAYLOAD_TEST
 payload_progress(stage,result);
#endif
 if(log_file){fprintf(log_file,"pkg-update-worker %s 0x%08X\n",stage,(unsigned)result);fflush(log_file);fsync(fileno(log_file));}
}
static int finish(int rc){if(log_file){fclose(log_file);log_file=NULL;}return rc;}
static void notify(const char *message){OrbisNotificationRequest n;memset(&n,0,sizeof(n));n.type=NotificationRequest;n.targetId=-1;snprintf(n.message,sizeof(n.message),"%s",message);sceKernelSendNotificationRequest(0,&n,sizeof(n),0);}
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
#ifndef HARBOR_PKG_PAYLOAD_TEST
static int read_sfo(const char *path,const char *key,char *out,size_t cap){
 unsigned char p[16384];FILE *f=fopen(path,"rb");if(!f)return -1;size_t n=fread(p,1,sizeof(p),f);int bad=ferror(f);fclose(f);
 return bad||n==sizeof(p)?-1:sfo_field(p,n,key,out,cap);
}
#endif
static uint32_t be32(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int modules(char *error,size_t cap){
 int rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_APP_INST_UTIL);
 if(rc<0)rc=(int32_t)sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx",0,NULL,0,NULL,NULL);
 link_stage("carregar AppInstUtil",rc);if(rc<0){snprintf(error,cap,"AppInstUtil indisponivel no Updater (0x%08X).",(unsigned)rc);return -1;}
 rc=sceAppInstUtilInitialize();link_stage("inicializar AppInstUtil",rc);
 if(rc){snprintf(error,cap,"AppInstUtil recusou a inicializacao (0x%08X).",(unsigned)rc);return -1;}return 0;
}
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
 if(!bad&&n>=8192){uint32_t entries=be32(p+0x10),table=be32(p+0x18);
  if(table<=n&&entries<=(n-table)/32)for(uint32_t i=0;i<entries;i++){
   const unsigned char *e=p+table+32*i;if(be32(e)!=0x1000)continue;
   uint32_t off=be32(e+16),length=be32(e+20);if(off>n||length>n-off)break;
   UpdateManifest current={.size=(uint64_t)size};
   if(!sfo_field(p+off,length,"APP_VER",current.sfo,sizeof(current.sfo))&&strlen(current.sfo)==5&&current.sfo[2]=='.'&&current.sfo[0]>='0'&&current.sfo[0]<='9'&&current.sfo[1]>='0'&&current.sfo[1]<='9'&&current.sfo[3]>='0'&&current.sfo[3]<='9'&&current.sfo[4]>='0'&&current.sfo[4]<='9'&&!update_pkg_metadata(p,n,(uint64_t)size,&current)){memcpy(installed_version,current.sfo,6);valid=1;}
  }
 }
 free(p);if(valid)return 0;
failed:
 snprintf(error,cap,"Nao foi possivel confirmar a versao atualmente instalada. Nada foi instalado.");return -1;
}
static int installed_sfo(char out[6],char *error,size_t cap){int rc=installer_with_access(installed_read,error,cap);if(!rc)memcpy(out,installed_version,6);return rc;}
static int32_t probe_pid;static int probe_result;
static int probe_process(char *error,size_t cap){
 if(!kill((pid_t)probe_pid,0)){probe_result=1;return 0;}
 if(errno==ESRCH){probe_result=0;return 0;}
 probe_result=-1;snprintf(error,cap,"Nao foi possivel consultar o processo anterior.");return -1;
}
static int alive(int32_t pid){probe_pid=pid;probe_result=-1;char error[128];return installer_with_access(probe_process,error,sizeof(error))?-1:probe_result;}
static void wait_ms(unsigned ms){usleep(ms*1000);}
static int ack(const char *nonce){
 FILE *f=fopen(PKG_UPDATE_ROOT "/ack.part","wb");if(!f)return -1;
 int bad=fwrite(nonce,1,32,f)!=32||fflush(f)||fsync(fileno(f));if(fclose(f))bad=1;
 return bad?-1:rename(PKG_UPDATE_ROOT "/ack.part",PKG_UPDATE_ROOT "/ack");
}
static const char *native_pkg_path;
static const UpdateManifest *install_candidate;
#ifdef HARBOR_PKG_PAYLOAD_TEST
static void report(const char *stage,int rc){link_stage(stage,rc);}
#define candidate (*install_candidate)
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
#undef candidate
#endif
static int install_call(void *unused,char *error,size_t cap){
 (void)unused;int rc=sceAppInstUtilInitialize();link_stage("inicializar instalacao autorizada",rc);
 if(rc){snprintf(error,cap,"AppInstUtil nao inicializou (0x%08X).",(unsigned)rc);return -1;}
 rc=sceAppInstUtilAppInstallPkg(native_pkg_path,NULL);link_stage("resultado AppInstallPkg apos fecho",rc);
 if(rc){snprintf(error,cap,"Instalacao recusada (0x%08X). O PKG original ficou em /data/pkg.",(unsigned)rc);return -1;}return 0;
}
static int install(const char *path,char *error,size_t cap){
 char system_path[780];
#ifdef HARBOR_PKG_PAYLOAD_TEST
 char copy[720];snprintf(copy,sizeof(copy),"%s.payload-install.pkg",path);
 if(copy_pkg(path,copy,error,cap))return -1;
 if(!payload_data_path(copy,system_path,sizeof(system_path))){snprintf(error,cap,"Caminho da copia de instalacao demasiado longo.");return -1;}
#else
 snprintf(system_path,sizeof(system_path),"/user%s",path);
#endif
 native_pkg_path=system_path;notify("h1pNoise: app fechada. A instalar a atualizacao verificada.");
 return installer_with_permissions(install_call,NULL,error,cap);
}
static const UpdateManifest *candidate;
static int confirm_installed(char *error,size_t cap){return update_file_verify("/user/app/HBRW00001/app.pkg",candidate,error,cap);}
static int installed_matches(const UpdateManifest *m,char *error,size_t cap){candidate=m;return installer_with_access(confirm_installed,error,cap);}
static int reopen(void *context,char *error,size_t cap){
 int rc=sceSystemServiceLaunchApp("HBRW00001",NULL,(LncAppParam*)context);link_stage("reabrir h1pNoise",rc);
 if(rc<0){snprintf(error,cap,"Atualizacao confirmada. Abre a h1pNoise no menu da PS4 (0x%08X).",(unsigned)rc);return -1;}return 0;
}
#ifdef HARBOR_PKG_PAYLOAD_TEST
int pkg_update_payload_main(void){
#else
int main(void){
#endif
 char error[512]={0};
#ifndef HARBOR_PKG_PAYLOAD_TEST
 char own_title[16]={0};
#endif
 unsigned char raw[UPDATE_MANIFEST_MAX+1],request_raw[65];size_t size=0,n=0;
 log_file=fopen("/data/pkg/update-install-debug.log","a");link_stage("auxiliar independente iniciou",0);
 #ifndef HARBOR_PKG_PAYLOAD_TEST
 /* Refuse to install if this binary was launched under the target title. */
 if(read_sfo("/app0/sce_sys/param.sfo","TITLE_ID",own_title,sizeof(own_title))||strcmp(own_title,PKG_UPDATER_TITLE)){snprintf(error,sizeof(error),"O instalador nao esta a correr como app independente.");goto failed;}
 #endif
 if(!access(PKG_UPDATE_ROOT "/claimed",F_OK)){snprintf(error,sizeof(error),"Ja existe um pedido entregue. Consulta update-install-debug.log antes de repetir.");goto failed;}
 if(rename(PKG_UPDATE_ROOT "/request",PKG_UPDATE_ROOT "/claimed")){snprintf(error,sizeof(error),"Nao existe um pedido novo. A h1pNoise prepara-o depois de descarregar.");goto failed;}
 FILE *f=fopen(PKG_UPDATE_ROOT "/claimed","rb");if(f){n=fread(request_raw,1,sizeof(request_raw),f);int bad=ferror(f);fclose(f);if(bad)n=0;}
 link_stage("bytes lidos do pedido",(int)n);
 PkgUpdateRequest request;if(pkg_update_request_read(request_raw,n,&request)||request.source_pid==(int32_t)getpid()){snprintf(error,sizeof(error),"Pedido de entrega invalido. Nada foi instalado.");goto failed_claimed;}
 f=fopen(PKG_UPDATE_ROOT "/manifest.h1p","rb");if(f){size=fread(raw,1,sizeof(raw),f);int bad=ferror(f);fclose(f);if(bad)size=0;}
 link_stage("bytes lidos do manifesto",(int)size);
 if(installer_with_access(modules,error,sizeof(error)))goto failed_claimed;
 UpdateManifest signed_candidate;
 if(update_manifest_read(raw,size,UPDATE_PUBLIC_KEY,&signed_candidate,error,sizeof(error)))goto failed_claimed;
 install_candidate=&signed_candidate;
 PkgUpdateOps ops={installed_sfo,alive,wait_ms,ack,install,installed_matches,link_stage};
 if(pkg_update_run(raw,size,UPDATE_PUBLIC_KEY,&request,&ops,error,sizeof(error)))goto failed_claimed;
 remove(PKG_UPDATE_ROOT "/claimed");remove(PKG_UPDATE_ROOT "/ack");
 notify("h1pNoise: nova instalacao confirmada. A abrir a app.");
 LncAppParam param={0};param.size=sizeof(param);int32_t user;
 if(!ps4_active_user(&user,error,sizeof(error))){param.user_id=(uint32_t)user;if(!installer_with_permissions(reopen,&param,error,sizeof(error)))return finish(0);}
 notify(*error?error:"Atualizacao confirmada. Abre a h1pNoise no menu da PS4.");return finish(0);
failed_claimed:
 /* Keep a claimed request if the native install was accepted but could not
    be confirmed. Requires examining the log rather than automatic retries. */
 link_stage("falha; PKG preservado",-1);
failed:
#ifdef HARBOR_PKG_PAYLOAD_TEST
 payload_progress(*error?error:"Falha sem detalhe no instalador",1);
#endif
 if(log_file){fprintf(log_file,"%s\n",error);fflush(log_file);fsync(fileno(log_file));fclose(log_file);log_file=NULL;}
 notify(*error?error:"O Updater nao concluiu a instalacao. O PKG foi preservado.");return finish(1);
}
