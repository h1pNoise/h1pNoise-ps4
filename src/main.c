#include "app.h"
#include "appearance.h"
#include <signal.h>
#ifdef HARBOR_RUNTIME_UPDATES
#include "runtime_update.h"
#include "version.h"
#endif
int main(int argc,char **argv){
#ifdef HARBOR_RUNTIME_UPDATES
 runtime_log("aplicacao iniciou",APP_BUILD);
#endif
 memset(&app,0,sizeof(app));mutex_init(&app.mu);mutex_init(&app.io);app.port=8787;app.direct_task=-1;app.update.task=-1;
#ifdef _WIN32
 snprintf(app.root,sizeof(app.root),"%s",argc>1?argv[1]:"harbor-data");if(argc>2)app.port=atoi(argv[2]);
#else
 (void)argc;(void)argv;strcpy(app.root,"/data/pkg");signal(SIGPIPE,SIG_IGN);
#endif
 make_dir(app.root);app.accent=accent_load(app.root);int rc=platform_init(app.ip);uint16_t secret;
 /* Rejection sampling avoids favouring some four-digit codes. */
 do{if(random_bytes(&secret,sizeof(secret)))return 1;}while(secret>=60000);
 snprintf(app.pin,sizeof(app.pin),"%04u",(unsigned)(secret%10000));
 memcpy(app.peer_id,"-HB0100-",8);if(random_bytes(app.peer_id+8,12))return 1;
 set_status("idle",rc?"Liga a PS4 a rede e volta a abrir a aplicacao.":"Envia um torrent pelo telemovel para comecar.");
 char current[700];snprintf(current,sizeof(current),"%s/current.txt",app.root);FILE *f=fopen(current,"rb");char hash[41]={0};if(f){size_t n=fread(hash,1,40,f);fclose(f);int good=n==40;for(int i=0;i<40&&good;i++)if(!((hash[i]>='0'&&hash[i]<='9')||(hash[i]>='a'&&hash[i]<='f')))good=0;
  if(good){snprintf(current,sizeof(current),"%s/%s/source.torrent",app.root,hash);f=fopen(current,"rb");if(f){fseeko(f,0,SEEK_END);int64_t len=ftello(f);fseeko(f,0,SEEK_SET);if(len>0&&len<=MAX_TORRENT){unsigned char *buf=malloc(len);if(buf){if(fread(buf,1,len,f)==(size_t)len){char err[512];fclose(f);f=NULL;import_torrent(buf,len,err,sizeof(err));}free(buf);}}if(f)fclose(f);}}
 }
 Thread server;if(thread_start(&server,http_server,NULL))return 1;
 updater_init();
#ifdef _WIN32
 printf("h1pNoise http://%s:%d CODE %s\n",app.ip,app.port,app.pin);fflush(stdout);thread_join(server);
#else
 screen_run(app.ip,app.pin);
#endif
 return 0;
}
