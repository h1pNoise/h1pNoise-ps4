#include "app.h"
#include "updater.h"
#include "update_config.h"
#include "version.h"
#include "vendor/monocypher-ed25519.h"
typedef struct {unsigned char data[UPDATE_MANIFEST_MAX];size_t used;} ManifestBuffer;
typedef struct {FILE *file;crypto_sha512_ctx hash;uint64_t done;} Download;
static void status(const char *phase,const char *message){lock(&app.mu);snprintf(app.update.phase,sizeof(app.update.phase),"%s",phase);snprintf(app.update.message,sizeof(app.update.message),"%s",message);unlock(&app.mu);}
static int manifest_sink(const unsigned char *p,size_t n,void *arg){ManifestBuffer *b=arg;if(n>sizeof(b->data)-b->used)return -1;memcpy(b->data+b->used,p,n);b->used+=n;return 0;}
static int file_sink(const unsigned char *p,size_t n,void *arg){Download *d=arg;if(fwrite(p,1,n,d->file)!=n)return -1;crypto_sha512_update(&d->hash,p,n);d->done+=n;lock(&app.mu);app.update.done=d->done;unlock(&app.mu);return 0;}
static void path_for(uint32_t build,char out[700]){snprintf(out,700,"%s/h1pNoise-update-%u.pkg",app.root,build);}
static int verify_file(const char *path,const UpdateManifest *m,char *error,size_t cap){
 FILE *f=fopen(path,"rb");if(!f){snprintf(error,cap,"A atualizacao descarregada ja nao esta disponivel.");return -1;}
 unsigned char buffer[32768],digest[64];crypto_sha512_ctx hash;crypto_sha512_init(&hash);uint64_t total=0;size_t n;
 while((n=fread(buffer,1,sizeof(buffer),f))){if(n>m->size-total){fclose(f);snprintf(error,cap,"O tamanho da atualizacao mudou.");return -1;}total+=n;crypto_sha512_update(&hash,buffer,n);}
 int io_error=ferror(f);crypto_sha512_final(&hash,digest);
 if(io_error||total!=m->size||crypto_verify64(digest,m->sha512)){fclose(f);snprintf(error,cap,"O ficheiro da atualizacao nao corresponde a versao assinada.");return -1;}
 /* The PKG header/SFO live in the bounded metadata area, before the PFS. */
 unsigned char *meta=malloc(512*1024);if(!meta){fclose(f);snprintf(error,cap,"Sem memoria para verificar a atualizacao.");return -1;}
 rewind(f);n=fread(meta,1,512*1024,f);int bad=ferror(f)||update_pkg_metadata(meta,n,total,m);free(meta);fclose(f);
 if(bad){snprintf(error,cap,"O pacote nao corresponde a identidade e versao da h1pNoise.");return -1;}return 0;
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
  if(m.build<=APP_BUILD){lock(&app.mu);app.update.available=app.update.ready=0;memset(&app.update.manifest,0,sizeof(app.update.manifest));unlock(&app.mu);status("current","Tens a versao mais recente publicada.");rc=0;goto done;}
  if(strcmp(m.sfo,APP_SFO_VERSION)<=0){snprintf(error,sizeof(error),"A versao PS4 publicada nao e mais recente.");goto done;}
  lock(&app.mu);app.update.manifest=m;app.update.available=1;app.update.ready=0;int notify=app.update.notified!=m.build;app.update.notified=m.build;unlock(&app.mu);
  char msg[180];snprintf(msg,sizeof(msg),"h1pNoise %s disponivel. Abre Atualizacoes no telemovel.",m.version);status("available",msg);if(notify)update_notify(msg);rc=0;
 }else{
  lock(&app.mu);m=app.update.manifest;unlock(&app.mu);char path[700];path_for(m.build,path);
  if(op==1){
   if(storage_check(app.root,m.size+64*1024*1024ULL,error,sizeof(error)))goto done;
   char part[720];snprintf(part,sizeof(part),"%s.part",path);Download d={0};d.file=fopen(part,"wb");if(!d.file){snprintf(error,sizeof(error),"Nao foi possivel criar o ficheiro da atualizacao.");goto done;}
   crypto_sha512_init(&d.hash);int net=update_http_get(m.url,m.size,file_sink,&d,error,sizeof(error));int io=fflush(d.file);if(fclose(d.file))io=-1;
   unsigned char digest[64];crypto_sha512_final(&d.hash,digest);
   if(net||io||d.done!=m.size||crypto_verify64(digest,m.sha512)){remove(part);if(!*error)snprintf(error,sizeof(error),"Download incompleto ou corrompido. A versao instalada nao foi alterada.");goto done;}
   if(verify_file(part,&m,error,sizeof(error))){remove(part);goto done;}
   /* Only our deterministic update file can be replaced; never torrent content. */
#ifdef _WIN32
   remove(path);
#endif
   if(rename(part,path)){snprintf(error,sizeof(error),"Nao foi possivel guardar a atualizacao verificada.");goto done;}
   lock(&app.mu);app.update.ready=1;unlock(&app.mu);
   char msg[960];snprintf(msg,sizeof(msg),"Atualizacao descarregada e verificada em %s. Fecha a h1pNoise e instala o PKG manualmente.",path);status("ready",msg);rc=0;
  }else{
   if(verify_file(path,&m,error,sizeof(error))){lock(&app.mu);app.update.ready=0;unlock(&app.mu);goto done;}
   int task=-1;rc=update_platform_install(path,&task,error,sizeof(error));lock(&app.mu);app.update.task=task;unlock(&app.mu);
   if(!rc){status("queued","Atualizacao enviada ao sistema. Fecha a h1pNoise e acompanha em Notificacoes > Transferencias. Reabre depois de concluir.");update_notify("h1pNoise: fecha a app para concluir a atualizacao nas Transferencias.");}
  }
 }
done:
 if(rc){status("error",*error?error:"Nao foi possivel concluir a atualizacao.");}
 lock(&app.mu);app.update.busy=0;unlock(&app.mu);return NULL;
}
int updater_begin(int op,char *error,size_t cap){
 if(op<0||op>2||!updater_supported()){snprintf(error,cap,"As atualizacoes da aplicacao requerem uma PS4 real.");return -1;}
 lock(&app.mu);
 if(app.busy||app.direct_busy||app.update.busy||app.update.task>=0){unlock(&app.mu);snprintf(error,cap,"Aguarda pela operacao atual. Se ja enviaste a atualizacao, consulta as Transferencias da PS4.");return -1;}
 if(op&&(!app.update.available||app.update.manifest.build<=APP_BUILD||(op==2&&!app.update.ready))){unlock(&app.mu);snprintf(error,cap,"Verifica e descarrega uma atualizacao valida primeiro.");return -1;}
 app.update.busy=1;if(op==1)app.update.done=0;unlock(&app.mu);
 status(op==0?"checking":op==1?"downloading":"installing",op==0?"A procurar atualizacoes...":op==1?"A descarregar a atualizacao...":"A verificar e preparar a instalacao...");
 Thread t;if(thread_start(&t,worker,(void*)(intptr_t)op)){lock(&app.mu);app.update.busy=0;unlock(&app.mu);snprintf(error,cap,"Nao foi possivel iniciar a atualizacao.");status("error",error);return -1;}
#ifdef _WIN32
 CloseHandle(t);
#else
 pthread_detach(t);
#endif
 return 0;
}
void updater_init(void){app.update.task=-1;status("idle",updater_supported()?"Verifica se existe uma nova versao.":"As atualizacoes da aplicacao requerem uma PS4 real.");if(updater_supported()){char error[512]={0};updater_begin(0,error,sizeof(error));}}
