#include "runtime_update.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <orbis/libkernel.h>
extern int32_t sceSystemServiceLoadExec(const char *,const char **);
int main(void){
 char path[700];int choice=runtime_select(path);
 runtime_log(choice==1?"abrir candidata":choice==2?"abrir ultima confirmada":"abrir versao do PKG",0);
 int rc=sceSystemServiceLoadExec(path,NULL);
 runtime_log("LoadExec regressou",rc);
 /* LoadExec normally never returns. A rejected candidate does not alter
    the installed PKG; reload the selector (attempt forces the fallback). */
 if(choice==1){runtime_select(path);runtime_log("abrir alternativa",0);rc=sceSystemServiceLoadExec(path,NULL);runtime_log("alternativa regressou",rc);}
 if(strcmp(path,"/app0/h1pNoise.self")){rc=sceSystemServiceLoadExec("/app0/h1pNoise.self",NULL);runtime_log("versao do PKG regressou",rc);}
 OrbisNotificationRequest r;memset(&r,0,sizeof(r));r.type=NotificationRequest;r.targetId=-1;
 snprintf(r.message,sizeof(r.message),"h1pNoise: nao foi possivel abrir a app (0x%08X). O PKG instalado mantem-se intacto.",(unsigned)rc);sceKernelSendNotificationRequest(0,&r,sizeof(r),0);
 return 1;
}
