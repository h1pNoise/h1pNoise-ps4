#include "app.h"
#include "pkg_update.h"
#include "update_config.h"
#include "version.h"
extern const unsigned char update_payload[];
extern const size_t update_payload_size;
static int store(const char *path,const void *data,size_t size){
 FILE *f=fopen(path,"wb");if(!f)return -1;
 int bad=fwrite(data,1,size,f)!=size||fflush(f)||fsync(fileno(f));if(fclose(f))bad=1;return bad?-1:0;
}
static void report(const char *message){
 FILE *f=fopen("/data/pkg/update-install-debug.log","a");
 if(f){fprintf(f,"pkg-payload-handoff %s\n",message);fflush(f);fsync(fileno(f));fclose(f);}
}
int pkg_update_payload_handoff(const char *path,char *error,size_t cap){
 unsigned char raw[UPDATE_MANIFEST_MAX],bytes[16];size_t n;
 lock(&app.mu);n=app.update.signed_size;if(n<=sizeof(raw))memcpy(raw,app.update.signed_manifest,n);unlock(&app.mu);
 UpdateManifest m;char expected[700];
 if(!n||n>sizeof(raw)||update_manifest_read(raw,n,UPDATE_PUBLIC_KEY,&m,error,cap))return -1;
 snprintf(expected,sizeof(expected),"/data/pkg/h1pNoise-update-%u.pkg",m.build);
 if(strcmp(path,expected)||m.build<=APP_BUILD||strcmp(m.sfo,APP_SFO_VERSION)<=0){snprintf(error,cap,"Atualizacao ou destino invalido.");return -1;}
 if(update_file_verify(path,&m,error,cap)||random_bytes(bytes,sizeof(bytes)))return -1;
 char nonce[33],request[64];for(unsigned i=0;i<16;i++)snprintf(nonce+2*i,3,"%02x",bytes[i]);
 int length=snprintf(request,sizeof(request),"%d\n%s\n",(int)getpid(),nonce);
 mkdir("/data/harbor",0777);mkdir(PKG_UPDATE_ROOT,0777);
 if(!access(PKG_UPDATE_ROOT "/claimed",F_OK)){snprintf(error,cap,"Ja existe uma entrega ao payload. Consulta update-install-debug.log antes de repetir.");return -1;}
 int fd=open(PKG_UPDATE_ROOT "/request",O_WRONLY|O_CREAT|O_EXCL,0600);
 if(fd<0){snprintf(error,cap,"Ja existe um pedido pendente. A app e o PKG foram preservados; envia update-install-debug.log.");return -1;}
 close(fd);remove(PKG_UPDATE_ROOT "/ack");
 if(store(PKG_UPDATE_ROOT "/manifest.h1p",raw,n)||store(PKG_UPDATE_ROOT "/request",request,(size_t)length)){
  remove(PKG_UPDATE_ROOT "/request");snprintf(error,cap,"Nao foi possivel guardar a entrega ao payload.");return -1;
 }
 /* Only the local, explicitly enabled GoldHEN loader; no LAN scanning or
    fallback to other exploit ports, and no unsigned externally supplied code. */
 Sock sock=tcp_connect("127.0.0.1",9090,5);
 if(sock==BADSOCK){remove(PKG_UPDATE_ROOT "/request");report("GoldHEN 9090 indisponivel; app aberta");snprintf(error,cap,"Liga Enable BinLoader Server no GoldHEN (9090). A app manteve-se aberta e o PKG ficou guardado.");return -1;}
 int sent=send_all(sock,update_payload,update_payload_size);sockclose(sock);
 lock(&app.mu);app.update.install_sent=1;unlock(&app.mu);
 report(sent?"envio incompleto; nao repetir sem verificar pedido":"payload enviado; aguardar confirmacao independente");
 /* Closing the socket delivers EOF. Bytes sent alone do not prove execution.
    Close our PID only after the independent verifier writes this exact nonce. */
 for(unsigned i=0;i<120;i++){
  char ack[34]={0};FILE *f=fopen(PKG_UPDATE_ROOT "/ack","rb");size_t used=0;
  if(f){used=fread(ack,1,sizeof(ack),f);int bad=ferror(f);fclose(f);if(bad)used=0;}
  if(used==32&&!memcmp(ack,nonce,32)){
   report("PKG confirmado pelo payload; fechar apenas esta app");
   update_notify("h1pNoise: download verificado. A app vai fechar para instalar a atualizacao.");_exit(0);
  }
  sleep_ms(500);
 }
 snprintf(error,cap,"O payload nao confirmou a entrega. A app continua aberta e o PKG foi preservado. Envia update-install-debug.log.");return -1;
}
