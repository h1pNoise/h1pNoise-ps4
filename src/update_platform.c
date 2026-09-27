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
extern int ps4_installer_ready(char*,size_t);
typedef struct {int32_t user,entitlement;const char *id,*url,*ex_url,*name,*icon,*sku;uint32_t options;const char *playgo,*release,*type,*subtype;uint64_t size;} UpdateBgftParam;
typedef struct {UpdateBgftParam params;uint32_t slot;} UpdateBgftEx;
_Static_assert(offsetof(UpdateBgftEx,slot)==104,"BGFT local install ABI");
int update_platform_install(const char *path,int *task,char *error,size_t cap){
 *task=-1;int rc;if(strncmp(path,"/data/pkg/h1pNoise-update-",sizeof("/data/pkg/h1pNoise-update-")-1)){snprintf(error,cap,"Caminho de atualizacao invalido.");return -1;}
 if(ps4_installer_ready(error,cap))return -1;
 char title[18]={0};int is_app=0;rc=sceAppInstUtilGetTitleIdFromPkg(path,title,&is_app);
 if(rc||strcmp(title,APP_TITLE_ID)){snprintf(error,cap,"O instalador nao reconheceu uma atualizacao da h1pNoise.");return -1;}
 UpdateBgftEx p={0};char storage[760];snprintf(storage,sizeof(storage),"/user%s",path);
 rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE);if(rc<0)goto failed;
 rc=sceUserServiceGetForegroundUser(&p.params.user);if(rc)goto failed;
 p.params.entitlement=5;p.params.id=APP_CONTENT_ID;p.params.url=storage;p.params.name="h1pNoise - atualizacao";p.params.icon="";p.params.playgo="0";p.params.options=ORBIS_BGFT_TASK_OPT_FORCE_UPDATE;
 int id=-1;rc=sceBgftServiceIntDownloadRegisterTaskByStorageEx((OrbisBgftDownloadParamEx*)&p,&id);
 /* Never uninstall the running app as a fallback. Keep the verified PKG for manual installation. */
 if(rc||id<0)goto failed;
 *task=id;rc=sceBgftServiceDownloadStartTask(id);
 if(rc){snprintf(error,cap,"Atualizacao registada como pedido %d, mas nao iniciou (0x%08X). Consulta as Transferencias antes de repetir.",id,(unsigned)rc);return -1;}
 return 0;
failed:
 snprintf(error,cap,"A PS4 recusou a instalacao (0x%08X). Fecha a app e instala manualmente o PKG verificado: %s. Nada foi desinstalado.",(unsigned)rc,path);return -1;
}
void update_notify(const char *message){OrbisNotificationRequest r;memset(&r,0,sizeof(r));r.type=NotificationRequest;r.targetId=-1;snprintf(r.message,sizeof(r.message),"%s",message);sceKernelSendNotificationRequest(0,&r,sizeof(r),0);}
#else
int update_platform_install(const char *p,int *task,char *e,size_t n){(void)p;*task=-1;snprintf(e,n,"Instalacao de atualizacoes disponivel apenas numa PS4 real.");return -1;}
void update_notify(const char *message){(void)message;}
#endif
