#include "app.h"
#include "pkg_update.h"
#include "ps4_user.h"
#include "installer_access.h"
#include <stdbool.h>
#include <orbis/AppInstUtil.h>
#include <orbis/_types/sys_service.h>
extern int ps4_installer_ready(char *,size_t);
extern int32_t sceSystemServiceLaunchApp(const char *,const char **,LncAppParam *);
static int write_file(const char *path,const void *data,size_t n){
 FILE *f=fopen(path,"wb");if(!f)return -1;
 int bad=fwrite(data,1,n,f)!=n||fflush(f)||fsync(fileno(f));if(fclose(f))bad=1;return bad?-1:0;
}
static int launch(void *context,char *error,size_t cap){
 LncAppParam *param=context;int rc=sceSystemServiceLaunchApp(PKG_UPDATER_TITLE,NULL,param);
 if(rc<0){snprintf(error,cap,"Nao foi possivel abrir h1pNoise Updater (0x%08X). O PKG foi preservado. Instala manualmente com a app fechada.",(unsigned)rc);return -1;}return 0;
}
int pkg_update_handoff(const char *path,char *error,size_t cap){
 unsigned char signed_manifest[UPDATE_MANIFEST_MAX],bytes[16];size_t size;
 lock(&app.mu);size=app.update.signed_size;if(size<=sizeof(signed_manifest))memcpy(signed_manifest,app.update.signed_manifest,size);uint32_t build=app.update.manifest.build;unlock(&app.mu);
 char expected[700];snprintf(expected,sizeof(expected),"/data/pkg/h1pNoise-update-%u.pkg",build);
 if(strcmp(path,expected)||!size||size>sizeof(signed_manifest)){snprintf(error,cap,"Pedido de atualizacao invalido.");return -1;}
 if(ps4_installer_ready(error,cap))return -1;
 int exists=0,rc=sceAppInstUtilAppExists(PKG_UPDATER_TITLE,&exists);
 if(rc||!exists){snprintf(error,cap,"Instala primeiro a app auxiliar h1pNoise Updater. O PKG foi preservado e tambem pode ser instalado pelo GoldHEN.");return -1;}
 LncAppParam param={0};param.size=sizeof(param);int32_t user;
 if(ps4_active_user(&user,error,cap)||random_bytes(bytes,sizeof(bytes)))return -1;param.user_id=(uint32_t)user;
 char nonce[33],request[64];for(int i=0;i<16;i++)snprintf(nonce+2*i,3,"%02x",bytes[i]);
 int length=snprintf(request,sizeof(request),"%d\n%s\n",(int)getpid(),nonce);
 mkdir("/data/harbor",0777);mkdir(PKG_UPDATE_ROOT,0777);
 /* Existing claimed request means the independent installer owns the work.
    Never overwrite it or retry a possibly accepted system request. */
 if(!access(PKG_UPDATE_ROOT "/claimed",F_OK)){snprintf(error,cap,"Ja existe um pedido entregue ao Updater. Consulta update-install-debug.log antes de repetir.");return -1;}
 int fd=open(PKG_UPDATE_ROOT "/request",O_WRONLY|O_CREAT|O_EXCL,0600);
 if(fd<0){snprintf(error,cap,"Ja existe um pedido pendente ou nao foi possivel guarda-lo. O PKG foi preservado.");return -1;}close(fd);
 remove(PKG_UPDATE_ROOT "/ack");
 if(write_file(PKG_UPDATE_ROOT "/manifest.h1p",signed_manifest,size)||write_file(PKG_UPDATE_ROOT "/request",request,(size_t)length)){
  remove(PKG_UPDATE_ROOT "/request");snprintf(error,cap,"Nao foi possivel guardar a entrega ao Updater.");return -1;
 }
 if(installer_with_permissions(launch,&param,error,cap)){
  /* A claimed request is retained if the launch returned after the helper
     began; deleting it would make an unsafe second attempt possible. */
  if(access(PKG_UPDATE_ROOT "/claimed",F_OK))remove(PKG_UPDATE_ROOT "/request");return -1;
 }
 for(unsigned i=0;i<30;i++){
  char ack[34]={0};FILE *f=fopen(PKG_UPDATE_ROOT "/ack","rb");size_t n=0;
  if(f){n=fread(ack,1,sizeof(ack),f);int bad=ferror(f);fclose(f);if(bad)n=0;}
  if(n==32&&!memcmp(ack,nonce,32)){
   update_notify("h1pNoise: PKG verificado e entregue ao Updater. A app vai fechar para instalar.");
   /* The helper has independently checked the signed PKG. It waits for this
      PID to disappear; no install is called by this running application. */
   _exit(0);
  }
  sleep_ms(500);
 }
 snprintf(error,cap,"O Updater nao confirmou a entrega. A h1pNoise manteve-se aberta e o PKG foi preservado. Consulta update-install-debug.log.");return -1;
}
