#include "app.h"
#include "remote_pkg.h"
#include "web.h"
#include "pairing.h"
#include "version.h"
#ifdef HARBOR_SHADPS4
#define STORAGE_UNCHECKED "true"
#else
#define STORAGE_UNCHECKED "false"
#endif
static void qr_response(Sock s);
static void reply(Sock s,int status,const char *type,const void *p,size_t n){
 char h[640];int k=snprintf(h,sizeof(h),"HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nX-Frame-Options: DENY\r\nContent-Security-Policy: default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'\r\n\r\n",status,status==200?"OK":"Error",type,n);if(!send_all(s,h,k))send_all(s,p,n);
}
static void fail(Sock s,int status,const char *msg){char escaped[1600],out[1700];jsonstr(escaped,sizeof(escaped),msg);snprintf(out,sizeof(out),"{\"error\":%s}",escaped);reply(s,status,"application/json; charset=utf-8",out,strlen(out));}
static void qr_response(Sock s){
 uint8_t qr[PAIRING_QR_BYTES];char out[1800];
 if(!pairing_qr(app.ip,app.port,app.pin,qr)){fail(s,503,"Liga a consola a rede e reinicia o Harbor.");return;}
 int size=qrcodegen_getSize(qr),n=snprintf(out,sizeof(out),"{\"size\":%d,\"modules\":\"",size);
 for(int y=0;y<size;y++)for(int x=0;x<size;x++)out[n++]=qrcodegen_getModule(qr,x,y)?'1':'0';
 out[n++]='"';out[n++]='}';reply(s,200,"application/json",out,n);
}
static int header(const char *h,const char *name,char *out,size_t cap){
 size_t len=strlen(name);const char *p=strstr(h,"\r\n");int count=0;
 while(p&&(p+=2)&&*p){const char *e=strstr(p,"\r\n");if(!e)break;if((size_t)(e-p)>len&&!strncasecmp(p,name,len)&&p[len]==':'){const char *v=p+len+1;while(v<e&&(*v==' '||*v=='\t'))v++;if((size_t)(e-v)>=cap||++count>1)return -1;memcpy(out,v,e-v);out[e-v]=0;}p=e;}
 return count;
}
static void status_response(Sock s){
 char *out=malloc(160000);if(!out){fail(s,500,"Sem memoria.");return;}char name[1600],msg[1800];size_t n;
 lock(&app.mu);jsonstr(name,sizeof(name),app.loaded?app.torrent.name:"");jsonstr(msg,sizeof(msg),app.message);
 n=snprintf(out,160000,"{\"name\":%s,\"message\":%s,\"phase\":\"%s\",\"busy\":%s,\"loaded\":%s,\"done\":%llu,\"total\":%llu,\"peers\":%d,\"installDone\":%llu,\"installTotal\":%llu,\"files\":[",name,msg,app.phase,app.busy?"true":"false",app.loaded?"true":"false",(unsigned long long)app.done,(unsigned long long)app.torrent.total,app.peers,(unsigned long long)app.install_done,(unsigned long long)app.install_total);
 if(app.loaded)for(int i=0;i<app.torrent.nfiles;i++){char f[3200];jsonstr(f,sizeof(f),app.torrent.files[i].name);n+=snprintf(out+n,160000-n,"%s{\"name\":%s,\"size\":%llu}",i?",":"",f,(unsigned long long)app.torrent.files[i].size);}
 char direct_msg[3200];jsonstr(direct_msg,sizeof(direct_msg),app.direct_message);
 n+=snprintf(out+n,160000-n,"],\"directSupported\":%s,\"directBusy\":%s,\"directTask\":%d,\"directMessage\":%s,\"directPhase\":\"%s\"",remote_pkg_supported()?"true":"false",app.direct_busy?"true":"false",app.direct_task,direct_msg,app.direct_phase[0]?app.direct_phase:"idle");
 char update_message[3200],notes[4800],update_version[200];jsonstr(update_message,sizeof(update_message),app.update.message);jsonstr(notes,sizeof(notes),app.update.manifest.notes);jsonstr(update_version,sizeof(update_version),app.update.manifest.version);
 n+=snprintf(out+n,160000-n,",\"update\":{\"supported\":%s,\"busy\":%s,\"available\":%s,\"ready\":%s,\"task\":%d,\"phase\":\"%s\",\"message\":%s,\"version\":%s,\"current\":\"" APP_VERSION "\",\"notes\":%s,\"done\":%llu,\"size\":%llu}",updater_supported()?"true":"false",app.update.busy?"true":"false",app.update.available?"true":"false",app.update.ready?"true":"false",app.update.task,app.update.phase,update_message,update_version,notes,(unsigned long long)app.update.done,(unsigned long long)app.update.manifest.size);
 unlock(&app.mu);
 uint64_t available=0;char free_json[32]="null",storage_path[3200];int known=!free_bytes(app.root,&available);
 if(known)snprintf(free_json,sizeof(free_json),"%llu",(unsigned long long)available);
 jsonstr(storage_path,sizeof(storage_path),app.root);
 n+=snprintf(out+n,160000-n,",\"free\":%s,\"storagePath\":%s,\"allowUnknownSpace\":" STORAGE_UNCHECKED ",\"version\":\"" APP_VERSION " experimental\"}",free_json,storage_path);
 reply(s,200,"application/json; charset=utf-8",out,n);free(out);
}
static void handle(Sock s){
 sock_timeout(s,8);char head[8192];size_t n=0;int ended=0;
 while(n+1<sizeof(head)){if(recv(s,head+n,1,0)!=1)return;n++;if(n>=4&&!memcmp(head+n-4,"\r\n\r\n",4)){ended=1;break;}}
 if(!ended){fail(s,431,"Cabecalho demasiado grande.");return;}head[n]=0;
 char method[12],path[160],version[20];if(sscanf(head,"%11s %159s %19s",method,path,version)!=3){fail(s,400,"Pedido invalido.");return;}
 char host[100],expected[100],origin[200],token[100],length[40],encoding[100];snprintf(expected,sizeof(expected),"%s:%d",app.ip,app.port);
 if(header(head,"Host",host,sizeof(host))!=1){fail(s,403,"Abre o endereco IP apresentado na PS4.");return;}
 char *hostport=strrchr(host,':');if(!hostport||atoi(hostport+1)!=app.port){fail(s,403,"Porta incorreta.");return;}
 int oc=header(head,"Origin",origin,sizeof(origin));char own[200];snprintf(own,sizeof(own),"http://%s",expected);if(oc<0||(oc&&strcmp(origin,own))){fail(s,403,"Origem recusada.");return;}
 if(!strcmp(method,"GET")&&!strcmp(path,"/")){reply(s,200,"text/html; charset=utf-8",web_html,sizeof(web_html)-1);return;}
 if(!strcmp(method,"GET")&&!strcmp(path,"/icon.png")){reply(s,200,"image/png",web_icon,sizeof(web_icon));return;}
 if(header(head,"X-Harbor-Code",token,sizeof(token))!=1||strcmp(token,app.pin)){fail(s,401,"Introduz o codigo apresentado na PS4.");return;}
 if(!strcmp(method,"GET")&&!strcmp(path,"/api/status")){status_response(s);return;}
 if(!strcmp(method,"GET")&&!strcmp(path,"/api/qr")){qr_response(s);return;}
 if(strcmp(method,"POST")){fail(s,405,"Metodo nao permitido.");return;}
 if(header(head,"Transfer-Encoding",encoding,sizeof(encoding))!=0){fail(s,400,"Transfer-Encoding nao suportado.");return;}
 size_t bytes=0;int lc=header(head,"Content-Length",length,sizeof(length));if(lc<0){fail(s,400,"Tamanho invalido.");return;}if(lc){if(!length[0]){fail(s,400,"Tamanho invalido.");return;}for(char *p=length;*p;p++){if(*p<'0'||*p>'9'||bytes>MAX_TORRENT/10){fail(s,413,"Ficheiro demasiado grande.");return;}bytes=bytes*10+*p-'0';}if(bytes>MAX_TORRENT){fail(s,413,"O torrent deve ter ate 8 MB.");return;}}
 char error[512]="Operacao indisponivel.";int rc=0;
 if(!strcmp(path,"/api/pkg-url")){
  if(!bytes||bytes>=PKG_URL_CAP){fail(s,400,"Cola um link direto HTTP ou HTTPS com menos de 2048 caracteres.");return;}
  char url[PKG_URL_CAP];if(recv_all(s,url,bytes))return;url[bytes]=0;rc=begin_remote_pkg(url,bytes,error,sizeof(error));
 }else if(!strcmp(path,"/api/torrent")){
  if(!bytes){fail(s,400,"Escolhe um ficheiro .torrent.");return;}unsigned char *buf=malloc(bytes);if(!buf){fail(s,500,"Sem memoria.");return;}if(recv_all(s,buf,bytes)){free(buf);return;}rc=import_torrent(buf,bytes,error,sizeof(error));free(buf);
 }else{
  if(bytes){fail(s,400,"Este pedido nao aceita conteudo.");return;}
  if(!strcmp(path,"/api/update/check"))rc=updater_begin(0,error,sizeof(error));
  else if(!strcmp(path,"/api/update/download"))rc=updater_begin(1,error,sizeof(error));
  else if(!strcmp(path,"/api/update/install"))rc=updater_begin(2,error,sizeof(error));
  else if(!strcmp(path,"/api/download"))rc=begin_download(0,error,sizeof(error));
  else if(!strcmp(path,"/api/download-install"))rc=begin_download(1,error,sizeof(error));
  else if(!strcmp(path,"/api/install"))rc=begin_install(error,sizeof(error));
  else if(!strcmp(path,"/api/pause")){lock(&app.mu);if(!strcmp(app.phase,"installing")){rc=-1;snprintf(error,sizeof(error),"A instalacao ja esta em curso. Aguarda a conclusao.");}else app.pause=1;unlock(&app.mu);}
  else if(!strcmp(path,"/api/reset")){lock(&app.mu);if(app.busy||app.direct_busy||app.update.busy||app.update.task>=0){rc=-1;snprintf(error,sizeof(error),"Pausa primeiro a tarefa atual.");}else{app.pause=1;app.busy=0;app.loaded=0;app.done=0;app.install_done=app.install_total=0;app.message[0]=0;app.phase[0]=0;}unlock(&app.mu);}
  else{fail(s,404,"Pedido desconhecido.");return;}
 }
 if(rc)fail(s,400,error);else reply(s,200,"application/json","{\"ok\":true}",11);
}
void *http_server(void *unused){
 (void)unused;Sock s=socket(AF_INET,SOCK_STREAM,0);if(s==BADSOCK){set_status("error","Nao foi possivel abrir o servidor.");return NULL;}int yes=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(void*)&yes,sizeof(yes));
 struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons(app.port);a.sin_addr.s_addr=htonl(INADDR_ANY);
 if(bind(s,(struct sockaddr*)&a,sizeof(a))||listen(s,8)){sockclose(s);set_status("error","A porta 8787 esta ocupada ou indisponivel.");return NULL;}
 for(;;){Sock client=accept(s,NULL,NULL);if(client==BADSOCK){sleep_ms(100);continue;}handle(client);sockclose(client);}return NULL;
}
