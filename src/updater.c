#include "app.h"
#include "updater.h"
#include "update_config.h"
#include "version.h"
#include "pkg_update.h"
#include "vendor/monocypher-ed25519.h"
#ifdef HARBOR_RUNTIME_UPDATES
#include "runtime_update.h"
#endif
typedef struct {unsigned char data[UPDATE_MANIFEST_MAX];size_t used;} ManifestBuffer;
typedef struct {FILE *file;crypto_sha512_ctx hash;uint64_t done;} Download;
static void status(const char *phase,const char *message){lock(&app.mu);snprintf(app.update.phase,sizeof(app.update.phase),"%s",phase);snprintf(app.update.message,sizeof(app.update.message),"%s",message);unlock(&app.mu);}
static int manifest_sink(const unsigned char *p,size_t n,void *arg){ManifestBuffer *b=arg;if(n>sizeof(b->data)-b->used)return -1;memcpy(b->data+b->used,p,n);b->used+=n;return 0;}
static int file_sink(const unsigned char *p,size_t n,void *arg){Download *d=arg;if(fwrite(p,1,n,d->file)!=n)return -1;crypto_sha512_update(&d->hash,p,n);d->done+=n;lock(&app.mu);app.update.done=d->done;unlock(&app.mu);return 0;}
static void path_for(uint32_t build,int destination,char out[700]){
#ifdef HARBOR_RUNTIME_UPDATES
 runtime_path(build,"self",out);
#else
 snprintf(out,700,"%s/h1pNoise-update-%u.pkg",update_destination_root(destination),build);
#endif
}
int updater_supported(void){
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 return 1;
#else
 return 0;
#endif
}
static void *worker(void *arg){
 int op=(int)(intptr_t)arg,rc=-1;char error[512]={0};UpdateManifest m;
 if(op==0){
  ManifestBuffer b={0};if(update_http_get(UPDATE_FEED_URL,sizeof(b.data),manifest_sink,&b,error,sizeof(error))||update_manifest_read(b.data,b.used,UPDATE_PUBLIC_KEY,&m,error,sizeof(error)))goto done;
  if(m.build<=APP_BUILD){lock(&app.mu);app.update.available=app.update.ready=0;memset(&app.update.manifest,0,sizeof(app.update.manifest));unlock(&app.mu);status(m.build<APP_BUILD?"channel-old":"current",m.build<APP_BUILD?"O canal anuncia uma versao anterior a instalada. Consulta as releases no GitHub.":"Tens a versao mais recente deste canal de atualizacoes.");rc=0;goto done;}
  if(strcmp(m.sfo,APP_SFO_VERSION)<=0){snprintf(error,sizeof(error),"A versao PS4 publicada nao e mais recente.");goto done;}
  lock(&app.mu);app.update.manifest=m;app.update.available=1;app.update.ready=0;memcpy(app.update.signed_manifest,b.data,b.used);app.update.signed_size=b.used;int notify=app.update.notified!=m.build;app.update.notified=m.build;unlock(&app.mu);
  char msg[180];
#ifdef HARBOR_RUNTIME_UPDATES
  snprintf(msg,sizeof(msg),"h1pNoise %s disponivel. Carrega X no comando para atualizar.",m.version);
#elif defined(HARBOR_PKG_DIRECT_TEST)
  snprintf(msg,sizeof(msg),"h1pNoise %s disponivel. Descarregar e instalar sem fechar (teste).",m.version);
#elif defined(HARBOR_PKG_PAYLOAD_TEST)
  snprintf(msg,sizeof(msg),"h1pNoise %s disponivel. Descarregar, fechar e instalar automaticamente (teste GoldHEN).",m.version);
#elif defined(HARBOR_PKG_INSTALLER_TEST)
  snprintf(msg,sizeof(msg),"h1pNoise %s disponivel. No telemovel, escolhe Descarregar e instalar (teste). Requer h1pNoise Updater.",m.version);
#else
  snprintf(msg,sizeof(msg),"h1pNoise %s disponivel. Descarrega o PKG e instala manualmente com a app fechada.",m.version);
#endif
  status("available",msg);if(notify)update_notify(msg);rc=0;
 }else{
  lock(&app.mu);m=app.update.manifest;int destination=app.update.destination;unlock(&app.mu);char path[700];path_for(m.build,destination,path);
  if(op==1){
#ifdef HARBOR_RUNTIME_UPDATES
   mkdir("/data/harbor",0777);mkdir(RUNTIME_ROOT,0777);
   if(storage_check(RUNTIME_ROOT,m.size+64*1024*1024ULL,error,sizeof(error)))goto done;
#else
   if(update_destination_prepare(destination,error,sizeof(error))||storage_check(update_destination_root(destination),m.size+64*1024*1024ULL,error,sizeof(error)))goto done;
#endif
   char part[720];snprintf(part,sizeof(part),"%s.part",path);Download d={0};d.file=fopen(part,"wb");if(!d.file){snprintf(error,sizeof(error),"Nao foi possivel guardar a atualizacao no destino escolhido. Confirma a pen e o acesso de escrita.");goto done;}
   crypto_sha512_init(&d.hash);int net=update_http_get(m.url,m.size,file_sink,&d,error,sizeof(error));int io=fflush(d.file);if(fclose(d.file))io=-1;
   unsigned char digest[64];crypto_sha512_final(&d.hash,digest);
   if(net||io||d.done!=m.size||crypto_verify64(digest,m.sha512)){remove(part);if(!*error)snprintf(error,sizeof(error),"Download incompleto ou falha de escrita. Confirma o destino e tenta novamente. A versao instalada nao foi alterada.");goto done;}
   if(update_file_verify(part,&m,error,sizeof(error))){remove(part);goto done;}
   /* Only our deterministic update file can be replaced; never torrent content. */
#ifdef _WIN32
   remove(path);
#endif
   if(rename(part,path)){snprintf(error,sizeof(error),"Nao foi possivel guardar a atualizacao verificada.");goto done;}
   lock(&app.mu);app.update.ready=1;snprintf(app.update.path,sizeof(app.update.path),"%s",path);unlock(&app.mu);
#ifdef HARBOR_RUNTIME_UPDATES
   if(runtime_store_manifest(app.update.signed_manifest,app.update.signed_size,m.build,error,sizeof(error))){lock(&app.mu);app.update.ready=0;unlock(&app.mu);goto done;}
   /* Download requested by X or the page includes activation. The installed
      PKG and the previous working executable are never removed. */
   int task=-1;rc=update_platform_install(path,&task,error,sizeof(error));
   if(!rc){status("queued","Atualizacao verificada. A app vai reiniciar com a nova versao.");update_notify("h1pNoise: a abrir a nova versao.");sleep_ms(2000);rc=update_platform_restart(error,sizeof(error));}
#elif defined(HARBOR_PKG_DIRECT_TEST)
   status("installing","PKG verificado. A instalar sem fechar a h1pNoise...");
   int task=-1;rc=update_platform_install(path,&task,error,sizeof(error));
   if(!rc){lock(&app.mu);app.update.ready=app.update.available=0;unlock(&app.mu);status("installed","PKG instalado e confirmado no disco. A app continua aberta; confirma a abertura da nova versao depois.");update_notify("h1pNoise: PKG instalado. Confirma a abertura da nova versao.");}
#elif defined(HARBOR_PKG_PAYLOAD_TEST)
   status("installing","PKG verificado. A entregar ao payload; a app fecha depois da confirmacao...");
   int task=-1;rc=update_platform_install(path,&task,error,sizeof(error));
#elif defined(HARBOR_PKG_INSTALLER_TEST)
   status("installing","PKG verificado. A entregar a instalacao ao h1pNoise Updater...");
   int task=-1;rc=update_platform_install(path,&task,error,sizeof(error));
#else
   char msg[960];snprintf(msg,sizeof(msg),"Atualizacao descarregada e verificada em %s. Fecha a h1pNoise e instala pelo Package Installer do GoldHEN, com Enable Background Installation desligado.",path);status("ready",msg);rc=0;
#endif
  }else{
   if(update_file_verify(path,&m,error,sizeof(error))){lock(&app.mu);app.update.ready=0;unlock(&app.mu);goto done;}
   int task=-1;rc=update_platform_install(path,&task,error,sizeof(error));lock(&app.mu);app.update.task=task;unlock(&app.mu);
#ifdef HARBOR_PKG_DIRECT_TEST
   if(!rc){lock(&app.mu);app.update.ready=app.update.available=0;unlock(&app.mu);status("installed","PKG instalado e confirmado no disco. A app continua aberta; confirma a abertura da nova versao depois.");}
#else
   if(!rc){status("queued","Atualizacao verificada. A app vai reiniciar com a nova versao.");sleep_ms(2000);rc=update_platform_restart(error,sizeof(error));}
#endif
  }
 }
done:
 if(rc){status("error",*error?error:"Nao foi possivel concluir a atualizacao.");}
 lock(&app.mu);app.update.busy=0;unlock(&app.mu);return NULL;
}
static int begin(int op,int destination,char *error,size_t cap){
 if(op<0||op>2||!updater_supported()){snprintf(error,cap,"As atualizacoes da aplicacao requerem uma PS4 real.");return -1;}
 if(op==1&&(destination<0||destination>2)){snprintf(error,cap,"Destino de atualizacao invalido.");return -1;}
#if defined(HARBOR_RUNTIME_UPDATES) || defined(HARBOR_PKG_DIRECT_TEST) || defined(HARBOR_PKG_PAYLOAD_TEST) || defined(HARBOR_PKG_INSTALLER_TEST)
 if(op==1&&destination){snprintf(error,cap,"Este canal de teste requer o disco interno.");return -1;}
#endif
 lock(&app.mu);
 if(app.update.install_sent){unlock(&app.mu);snprintf(error,cap,"O pedido de instalacao ja foi enviado. Confirma a nova versao quando voltares a abrir a app; nao repitas o pedido.");return -1;}
 if(app.busy||app.direct_busy||app.update.busy||app.update.task>=0){unlock(&app.mu);snprintf(error,cap,"Aguarda pela operacao atual. Se ja enviaste a atualizacao, consulta as Transferencias da PS4.");return -1;}
 if(op&&(!app.update.available||app.update.manifest.build<=APP_BUILD||(op==2&&!app.update.ready))){unlock(&app.mu);snprintf(error,cap,"Verifica e descarrega uma atualizacao valida primeiro.");return -1;}
 app.update.busy=1;if(op==1){app.update.destination=destination;app.update.done=0;app.update.ready=0;app.update.path[0]=0;}unlock(&app.mu);
 status(op==0?"checking":op==1?"downloading":"installing",op==0?"A procurar atualizacoes...":op==1?"A descarregar a atualizacao...":"A verificar e preparar a instalacao...");
 Thread t;if(thread_start(&t,worker,(void*)(intptr_t)op)){lock(&app.mu);app.update.busy=0;unlock(&app.mu);snprintf(error,cap,"Nao foi possivel iniciar a atualizacao.");status("error",error);return -1;}
#ifdef _WIN32
 CloseHandle(t);
#else
 pthread_detach(t);
#endif
 return 0;
}
int updater_begin(int op,char *error,size_t cap){return begin(op,0,error,cap);}
int updater_download_to(const char *id,char *error,size_t cap){return begin(1,update_destination_id(id),error,cap);}
void updater_init(void){app.update.task=-1;status("idle",updater_supported()?"Verifica se existe uma nova versao.":"As atualizacoes da aplicacao requerem uma PS4 real.");if(updater_supported()){char error[512]={0};updater_begin(0,error,sizeof(error));}}
