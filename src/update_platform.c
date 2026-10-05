#include "app.h"
#include "updater.h"
#include "pkg_update.h"
#ifdef HARBOR_RUNTIME_UPDATES
#include "runtime_update.h"
extern int32_t sceSystemServiceLoadExec(const char *,const char **);
#endif
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
#include <orbis/libkernel.h>
#endif
int update_platform_install(const char *path,int *task,char *error,size_t cap){
 *task=-1;
#if defined(HARBOR_PKG_INSTALLER_TEST)
 return pkg_update_handoff(path,error,cap);
#elif defined(HARBOR_RUNTIME_UPDATES)
 lock(&app.mu);uint32_t build=app.update.manifest.build;unlock(&app.mu);
 char expected[700];runtime_path(build,"self",expected);
 if(strcmp(path,expected)){snprintf(error,cap,"Destino da atualizacao invalido.");return -1;}
 return runtime_activate(build,error,cap);
#else
 /* Preparing overwrite of our own running title can remove it and terminate
    us before BGFT registration. Replacement needs an independent installer. */
 snprintf(error,cap,"Atualizacao guardada em %s. A instalacao pela propria app foi suspensa. Fecha a h1pNoise e instala o PKG manualmente.",path);
 return -1;
#endif
}
int update_platform_restart(char *error,size_t cap){
#if defined(HARBOR_RUNTIME_UPDATES) && defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 int rc=sceSystemServiceLoadExec("/app0/eboot.bin",NULL);
 snprintf(error,cap,"A atualizacao foi guardada, mas o reinicio falhou (0x%08X). Fecha e volta a abrir a h1pNoise.",(unsigned)rc);return -1;
#else
 snprintf(error,cap,"O reinicio da atualizacao requer uma PS4 real.");return -1;
#endif
}
void update_notify(const char *message){
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 OrbisNotificationRequest r;memset(&r,0,sizeof(r));r.type=NotificationRequest;r.targetId=-1;snprintf(r.message,sizeof(r.message),"%s",message);sceKernelSendNotificationRequest(0,&r,sizeof(r),0);
#else
 (void)message;
#endif
}
