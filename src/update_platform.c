#include "app.h"
#include "updater.h"
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
#include <orbis/libkernel.h>
#endif
int update_platform_install(const char *path,int *task,char *error,size_t cap){
 /* Preparing overwrite of our own running title can remove it and terminate
    us before BGFT registration. Replacement needs an independent installer. */
 *task=-1;
 snprintf(error,cap,"Atualizacao guardada em %s. A instalacao pela propria app foi suspensa. Fecha a h1pNoise e instala o PKG manualmente.",path);
 return -1;
}
void update_notify(const char *message){
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 OrbisNotificationRequest r;memset(&r,0,sizeof(r));r.type=NotificationRequest;r.targetId=-1;snprintf(r.message,sizeof(r.message),"%s",message);sceKernelSendNotificationRequest(0,&r,sizeof(r),0);
#else
 (void)message;
#endif
}
