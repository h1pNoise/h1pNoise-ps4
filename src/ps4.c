#include "app.h"
#include <stdbool.h>
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>
#include <orbis/Net.h>
#include <orbis/NetCtl.h>
#include <orbis/VideoOut.h>
#include <orbis/Bgft.h>
#include <orbis/AppInstUtil.h>
#include <orbis/SystemService.h>
#include "display.h"
#include "pairing.h"
#include "version.h"
#ifdef HARBOR_RUNTIME_UPDATES
#include "runtime_update.h"
#include "ps4_user.h"
#include <orbis/Pad.h>
#endif
extern int sceRandomGetRandomNumber(void *buf,size_t size);
static int net_pool=-1;
int platform_init(char ip[16]){
 sceNetInit();sceNetCtlInit();net_pool=sceNetPoolCreate("harbor",65536,0);
 sceSysmoduleLoadModule(ORBIS_SYSMODULE_RANDOM);OrbisNetCtlInfo info;memset(&info,0,sizeof(info));
 if(sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS,&info)<0){strcpy(ip,"0.0.0.0");return -1;}
 snprintf(ip,16,"%s",info.ip_address);return 0;
}
int random_bytes(void *p,size_t n){while(n){size_t k=n>64?64:n;if(sceRandomGetRandomNumber(p,k))return -1;p=(char*)p+k;n-=k;}return 0;}
int ps4_resolve(const char *host,struct in_addr *out){if(net_pool<0)return -1;int r=sceNetResolverCreate("harbor-dns",net_pool,0);if(r<0)return -1;OrbisNetInAddr a;int rc=sceNetResolverStartNtoa(r,host,&a,3*1000*1000,1,0);sceNetResolverDestroy(r);if(rc<0)return -1;memcpy(out,&a,4);return 0;}
/* OpenOrbis 0.5.3's public progress header has 32-bit byte counters.
   The PS4 ABI uses 64-bit unsigned long counters (flatz's documented ABI). */
typedef struct{uint32_t bits;int32_t error;uint64_t length,transferred,length_total,transferred_total;uint32_t index,total,seconds,total_seconds;int32_t preparing,copy;} Progress64;
_Static_assert(sizeof(Progress64)==64,"BGFT progress ABI");
#ifndef HARBOR_SHADPS4
#include "installer_access.h"
#include "ps4_bgft.h"
extern void link_stage(const char *stage,int result);
/* Callers serialize installer operations using app.busy/direct_busy. */
static int installer_init(char *error,size_t cap){
 static int initialized=0;static OrbisBgftInitParams init;int rc;
 if(!initialized){
  link_stage("carregar AppInstUtil",0);
  rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_APP_INST_UTIL);
  link_stage("resultado AppInstUtil",rc);
  if(rc<0){
   errno=0;int module_fd=open("/system/common/lib/libSceAppInstUtil.sprx",O_RDONLY);
   link_stage("acesso ao ficheiro AppInstUtil",module_fd<0?-errno:0);
   if(module_fd>=0){unsigned char magic[4];int bytes=read(module_fd,magic,sizeof(magic));
    link_stage("leitura ficheiro AppInstUtil",bytes);close(module_fd);}
   link_stage("carregar AppInstUtil pelo caminho",rc);
   rc=(int32_t)sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx",0,NULL,0,NULL,NULL);
   link_stage("resultado caminho AppInstUtil",rc);
   if(rc<0){snprintf(error,cap,"AppInstUtil nao carregou: 0x%08X. Acesso ao ficheiro: %s. Envia o link-debug.log.",(unsigned)rc,module_fd<0?"indisponivel para a app":"confirmado");return -1;}
  }
  link_stage("carregar BGFT",0);
  rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_BGFT);
  link_stage("resultado BGFT",rc);
  if(rc<0){
   link_stage("carregar BGFT pelo caminho",rc);
   rc=(int32_t)sceKernelLoadStartModule("/system/common/lib/libSceBgft.sprx",0,NULL,0,NULL,NULL);
   link_stage("resultado caminho BGFT",rc);
   if(rc<0){snprintf(error,cap,"BGFT nao carregou pelo caminho do sistema: 0x%08X.",(unsigned)rc);return -1;}
  }
  link_stage("inicializar AppInstUtil",0);
  rc=sceAppInstUtilInitialize();if(rc){snprintf(error,cap,"AppInstUtil indisponivel: 0x%08X.",(unsigned)rc);return -1;}
  link_stage("reservar memoria BGFT",rc);
  init.heapSize=1024*1024;init.heap=calloc(1,init.heapSize);if(!init.heap){snprintf(error,cap,"Sem memoria para o instalador.");return -1;}
  link_stage("inicializar BGFT",0);
  rc=sceBgftServiceIntInit(&init);link_stage("resultado inicializacao BGFT",rc);if(rc){free(init.heap);init.heap=NULL;snprintf(error,cap,"BGFT indisponivel: 0x%08X.",(unsigned)rc);return -1;}initialized=1;
 }
 return 0;
}
int ps4_installer_ready(char *error,size_t cap){
 return installer_with_access(installer_init,error,cap);
}
#endif
int install_pkg(const char *path,const char *name,char *error,size_t cap,void(*progress)(uint64_t,uint64_t)){
#ifdef HARBOR_SHADPS4
 (void)path;(void)name;(void)progress;
 snprintf(error,cap,"Este teste shadPS4 nao instala PKG. Valida a instalacao numa PS4 real.");return -1;
#else
 int rc;if(ps4_installer_ready(error,cap))return -1;
 unsigned char header[128];FILE *f=fopen(path,"rb");if(!f){snprintf(error,cap,"PKG inacessivel.");return -1;}size_t n=fread(header,1,128,f);fclose(f);
 if(n!=128||memcmp(header,"\x7f" "CNT",4)){snprintf(error,cap,"Cabecalho PKG invalido.");return -1;}
 char cid[37]={0};memcpy(cid,header+0x40,36);for(int i=0;i<36;i++)if(!((cid[i]>='A'&&cid[i]<='Z')||(cid[i]>='a'&&cid[i]<='z')||(cid[i]>='0'&&cid[i]<='9')||cid[i]=='-'||cid[i]=='_')){snprintf(error,cap,"Content ID invalido.");return -1;}
 char title[18]={0};int is_app=0;rc=sceAppInstUtilGetTitleIdFromPkg(path,title,&is_app);if(rc){snprintf(error,cap,"Nao foi possivel ler o PKG: 0x%08X.",(unsigned)rc);return -1;}
 int exists=0;rc=sceAppInstUtilAppExists(title,&exists);uint32_t flags=be32(header+0x78);int patch=(flags&(0x00100000u|0x40000000u|0x41000000u|0x60000000u))!=0;
 if(rc){snprintf(error,cap,"Nao foi possivel verificar o jogo instalado: 0x%08X.",(unsigned)rc);return -1;}
 if(patch&&!exists){snprintf(error,cap,"Instala primeiro o jogo base %s.",title);return -1;}
 /* Do not uninstall or replace an installed base automatically. */
 if(!patch&&exists){snprintf(error,cap,"O jogo base %s ja esta instalado. Envia um torrent apenas com a atualizacao, ou instala os PKG manualmente.",title);return -1;}
 uint64_t sz=0;f=fopen(path,"rb");if(f){if(!fseeko(f,0,SEEK_END)){int64_t length=ftello(f);if(length>0)sz=(uint64_t)length;}fclose(f);}
 if(!sz){snprintf(error,cap,"Nao foi possivel ler o tamanho do PKG.");return -1;}
 if(storage_check(app.root,sz+64*1024*1024ULL,error,cap))return -1;
 char storage[800];snprintf(storage,sizeof(storage),"/user%s",path);OrbisBgftDownloadParamEx params;memset(&params,0,sizeof(params));
 params.params.entitlementType=5;params.params.id=cid;params.params.contentUrl=storage;params.params.contentName=name;params.params.iconPath="";params.params.playgoScenarioId="0";params.params.option=ORBIS_BGFT_TASK_OPT_NONE;
 int task=-1;if(ps4_bgft_submit(&params,BGFT_SUBMIT_STORAGE,&task,error,cap))return -1;
 time_t start=time(NULL);for(;;){Progress64 p;memset(&p,0,sizeof(p));rc=sceBgftServiceDownloadGetProgress(task,(OrbisBgftTaskProgress*)&p);
  if(rc||p.error){snprintf(error,cap,"Erro na instalacao: 0x%08X. Consulta tambem as Notificacoes da PS4.",(unsigned)(rc?rc:p.error));return -1;}
  progress(p.transferred,p.length);if(p.length&&p.transferred>=p.length)return 0;
  if(time(NULL)-start>4*60*60){snprintf(error,cap,"A instalacao excedeu 4 horas. Consulta as Notificacoes antes de tentar novamente.");return -1;}
  sceSystemServicePowerTick();sleep_ms(1000);
 }
 #endif
}
#define WIDTH 1280
#define HEIGHT 720
void screen_run(const char *ip,const char *pin){
#ifdef HARBOR_RUNTIME_UPDATES
 int pad=-1;int32_t user;char pad_error[512];unsigned previous_buttons=0;
 if(!ps4_active_user(&user,pad_error,sizeof(pad_error))&&!scePadInit())pad=scePadOpen(user,ORBIS_PAD_PORT_TYPE_STANDARD,0,NULL);
#endif
 uint8_t qr[PAIRING_QR_BYTES];int has_qr=pairing_qr(ip,app.port,pin,qr);
 char notice[300];snprintf(notice,sizeof(notice),"h1pNoise: http://%s:%d | Codigo: %s",ip,app.port,pin);OrbisNotificationRequest req;memset(&req,0,sizeof(req));req.type=NotificationRequest;req.targetId=-1;snprintf(req.message,sizeof(req.message),"%s",notice);sceKernelSendNotificationRequest(0,&req,sizeof(req),0);
 int video=sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN,ORBIS_VIDEO_OUT_BUS_MAIN,0,NULL);void *mem=NULL;off_t offset=0;size_t bytes=8*1024*1024;int ok=video>=0;
 if(ok)ok=sceKernelAllocateDirectMemory(0,sceKernelGetDirectMemorySize(),bytes,0x200000,3,&offset)>=0;
 if(ok)ok=sceKernelMapDirectMemory(&mem,bytes,0x33,0,offset,0x200000)>=0;
 void *buffers[2]={mem,mem?(char*)mem+WIDTH*HEIGHT*4:NULL};OrbisVideoOutBufferAttribute attr;
 if(ok){sceVideoOutSetBufferAttribute(&attr,0x80000000,1,0,WIDTH,HEIGHT,WIDTH);ok=sceVideoOutRegisterBuffers(video,0,buffers,2,&attr)==0;}
 if(!ok){for(;;){sceSystemServicePowerTick();sleep_ms(1000);}}
 int index=0;int64_t frame=0;time_t space_at=0;uint64_t available=0;int space_known=0;
 for(;;){
#ifdef HARBOR_RUNTIME_UPDATES
  if(frame==20)runtime_boot_confirm(APP_BUILD);
  if(pad>=0){OrbisPadData input;memset(&input,0,sizeof(input));if(scePadReadState(pad,&input)>=0){
   unsigned pressed=input.buttons&~previous_buttons;previous_buttons=input.buttons;
   if(pressed&ORBIS_PAD_BUTTON_CROSS){lock(&app.mu);int update=app.update.available&&!app.update.busy&&!app.busy&&!app.direct_busy;int ready=app.update.ready;unlock(&app.mu);if(update){char error[512];updater_begin(ready?2:1,error,sizeof(error));}}
  }}
#endif
  if(!space_at||time(NULL)-space_at>=5){space_known=!free_bytes(app.root,&available);space_at=time(NULL);}
  DisplayState state;memset(&state,0,sizeof(state));
  state.space_known=space_known;state.available=available;state.frame=(int)(frame%1000000);state.port=app.port;
#ifdef HARBOR_SHADPS4
  state.emulator=1;
#endif
  snprintf(state.ip,sizeof(state.ip),"%s",ip);snprintf(state.pin,sizeof(state.pin),"%s",pin);
  lock(&app.mu);
  state.loaded=app.loaded||app.magnet_pending;state.busy=app.busy;state.installing=!strcmp(app.phase,"installing");
  state.done=state.installing?app.install_done:app.done;state.total=state.installing?app.install_total:app.torrent.total;
  state.peers=app.peers;state.direct_busy=app.direct_busy;state.direct_task=app.direct_task;
  snprintf(state.phase,sizeof(state.phase),"%s",app.phase);
  snprintf(state.direct_phase,sizeof(state.direct_phase),"%s",app.magnet_pending?"":app.direct_phase);
  state.update_available=app.update.available;
  state.update_active=(app.update.busy&&strcmp(app.update.phase,"checking"))||app.update.ready||app.update.task>=0;
  state.update_done=app.update.done;state.update_size=app.update.manifest.size;
  snprintf(state.update_version,sizeof(state.update_version),"%s",app.update.manifest.version);
  snprintf(state.update_message,sizeof(state.update_message),"%s",app.update.message);
  snprintf(state.name,sizeof(state.name),"%s",app.torrent.name);
  snprintf(state.message,sizeof(state.message),"%s",!app.busy&&!app.magnet_pending&&app.direct_phase[0]?app.direct_message:app.message);
  unlock(&app.mu);
  display_render(buffers[index],&state,qr,has_qr);
  if(sceVideoOutSubmitFlip(video,index,ORBIS_VIDEO_OUT_FLIP_VSYNC,frame)>=0){for(int i=0;i<200;i++){OrbisVideoOutFlipStatus s;if(!sceVideoOutGetFlipStatus(video,&s)&&s.flipArg==frame)break;sleep_ms(5);}}
  index^=1;frame++;sceSystemServicePowerTick();sleep_ms(250);
 }
}
