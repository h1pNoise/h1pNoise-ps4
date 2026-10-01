#include "app.h"
#include "updater.h"
#include "version.h"
#include <stdbool.h>
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
#include <orbis/libkernel.h>
#include <orbis/Bgft.h>
#include <orbis/AppInstUtil.h>
#include <orbis/UserService.h>
#include <orbis/Sysmodule.h>
#include "ps4_user.h"
#include "ps4_bgft.h"
extern int ps4_installer_ready(char*,size_t);
extern void link_stage(const char*,int);
typedef struct {int32_t user,entitlement;const char *id,*url,*ex_url,*name,*icon,*sku;uint32_t options;const char *playgo,*release,*type,*subtype;uint64_t size;} UpdateBgftParam;
typedef struct {UpdateBgftParam params;uint32_t slot;} UpdateBgftEx;
_Static_assert(offsetof(UpdateBgftEx,slot)==104,"BGFT local install ABI");
typedef struct {const char *path;UpdateBgftEx *params;} Replacement;
static int prepare_replacement(void *context,char *error,size_t cap){
 Replacement *r=context;int32_t slot=-1;
 link_stage("consultar slot da h1pNoise instalada",0);
 int rc=sceAppInstUtilGetPrimaryAppSlot(APP_TITLE_ID,&slot);
 link_stage("resultado slot da h1pNoise",rc);
 if(rc||slot<0){snprintf(error,cap,"Nao foi possivel consultar a h1pNoise instalada (0x%08X). Envia o link-debug.log.",(unsigned)rc);return -1;}
 r->params->slot=(uint32_t)slot;
 link_stage("preparar substituicao da h1pNoise",0);
 rc=sceAppInstUtilAppPrepareOverwritePkg(r->path);
 link_stage("resultado preparacao da h1pNoise",rc);
 if(rc){snprintf(error,cap,"A PS4 nao preparou a substituicao da h1pNoise (0x%08X). Fecha a app e instala o PKG manualmente.",(unsigned)rc);return -1;}
 return 0;
}
int update_platform_install(const char *path,int *task,char *error,size_t cap){
 *task=-1;int rc;if(strncmp(path,"/data/pkg/h1pNoise-update-",sizeof("/data/pkg/h1pNoise-update-")-1)){snprintf(error,cap,"Caminho de atualizacao invalido.");return -1;}
 link_stage("iniciar instalador da atualizacao",0);if(ps4_installer_ready(error,cap))return -1;
 link_stage("identificar PKG da atualizacao",0);
 char title[18]={0};int is_app=0;rc=sceAppInstUtilGetTitleIdFromPkg(path,title,&is_app);
 link_stage("resultado identidade da atualizacao",rc);
 if(rc||strcmp(title,APP_TITLE_ID)||is_app!=1){snprintf(error,cap,"O instalador nao reconheceu uma atualizacao da h1pNoise.");return -1;}
 UpdateBgftEx p={0};char storage[760];snprintf(storage,sizeof(storage),"/user%s",path);
 if(ps4_active_user(&p.params.user,error,cap))return -1;
 p.params.entitlement=5;p.params.id=APP_CONTENT_ID;p.params.url=storage;p.params.name="h1pNoise - atualizacao";p.params.icon="";p.params.playgo="0";p.params.options=ORBIS_BGFT_TASK_OPT_FORCE_UPDATE;
 /* Never uninstall the running app as a fallback. Keep the verified PKG for manual installation. */
 Replacement replacement={path,&p};
 return ps4_bgft_submit_update(&p,prepare_replacement,&replacement,task,error,cap);
}
void update_notify(const char *message){OrbisNotificationRequest r;memset(&r,0,sizeof(r));r.type=NotificationRequest;r.targetId=-1;snprintf(r.message,sizeof(r.message),"%s",message);sceKernelSendNotificationRequest(0,&r,sizeof(r),0);}
#else
int update_platform_install(const char *p,int *task,char *e,size_t n){(void)p;*task=-1;snprintf(e,n,"Instalacao de atualizacoes disponivel apenas numa PS4 real.");return -1;}
void update_notify(const char *message){(void)message;}
#endif
