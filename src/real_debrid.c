#include "app.h"
#include "real_debrid.h"
#include "rd_json.h"
#include "magnet.h"
#include "remote_pkg.h"
#include "version.h"
#include <ctype.h>
#define RD_BASE "https://api.real-debrid.com/rest/1.0/"
/* Only the private console configuration stores credentials. Never return them
   in status, logs, torrent files, browser storage or download requests. */
static char api_token[RD_TOKEN_CAP],job[65],job_hash[41],magnet_url[MAGNET_CAP],file_jobs[MAX_FILES][65];
void rd_trace(const char *event,int first,int second){
 char path[600],line[200];snprintf(path,sizeof(path),"%s/real-debrid-debug.log",app.root);
 int n=snprintf(line,sizeof(line),"build=%u event=%s first=%d second=%d\n",APP_BUILD,event,first,second);
 FILE *f=fopen(path,"ab");if(f){fwrite(line,1,(size_t)n,f);fclose(f);}
}
typedef struct {char *p;size_t n;} Reply;
static int collect(const void *p,size_t n,void *ctx){Reply *r=ctx;if(n>512*1024-r->n)return -1;memcpy(r->p+r->n,p,n);r->n+=n;r->p[r->n]=0;return 0;}
static int stopped(void){lock(&app.mu);int v=app.pause;unlock(&app.mu);return v;}
static int token_valid(const char *token,size_t n,char *error,size_t cap){
 if(n>=RD_TOKEN_CAP){snprintf(error,cap,"API Real-Debrid demasiado longa.");return -1;}
 for(size_t i=0;i<n;i++)if(!isalnum((unsigned char)token[i])&&token[i]!='-'&&token[i]!='_'&&token[i]!='.'){snprintf(error,cap,"Cola apenas a API Real-Debrid, sem espacos ou cabecalhos.");return -1;}
 if(n&&n<16){snprintf(error,cap,"A API Real-Debrid esta incompleta.");return -1;}
 return 0;
}
static int save_config(const char *token,size_t n,int enabled){
 char path[600],temp[600],data[RD_TOKEN_CAP+8];snprintf(path,sizeof(path),"%s/.h1pNoise-real-debrid.conf",app.root);snprintf(temp,sizeof(temp),"%s/.h1pNoise-real-debrid.tmp",app.root);
 memcpy(data,"H1RD1\n0\n",8);data[6]=enabled?'1':'0';memcpy(data+8,token,n);int bad=0;
#ifdef _WIN32
 FILE *f=fopen(temp,"wb");if(!f)return -1;bad=fwrite(data,1,n+8,f)!=n+8;if(fflush(f))bad=1;if(fclose(f))bad=1;
 if(!bad)bad=!MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
#else
 /* Exclusive creation prevents following a pre-existing temporary symlink. */
 unlink(temp);int fd=open(temp,O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)return -1;
 size_t at=0;while(at<n+8){ssize_t written=write(fd,data+at,n+8-at);if(written<=0){bad=1;break;}at+=(size_t)written;}
 if(fsync(fd))bad=1;if(close(fd))bad=1;if(!bad)bad=rename(temp,path)!=0;
#endif
 memset(data,0,sizeof(data));if(bad)remove(temp);return bad?-1:0;
}
void rd_load(void){
 memset(api_token,0,sizeof(api_token));memset(file_jobs,0,sizeof(file_jobs));job[0]=job_hash[0]=magnet_url[0]=0;app.rd_enabled=app.rd_active=app.rd_configured=app.rd_magnet=0;
 char path[600],data[RD_TOKEN_CAP+9],error[128];snprintf(path,sizeof(path),"%s/.h1pNoise-real-debrid.conf",app.root);FILE *f=fopen(path,"rb");if(!f)return;
 size_t n=fread(data,1,sizeof(data),f);int bad=ferror(f);fclose(f);
 if(!bad&&n>=8&&n<8+RD_TOKEN_CAP&&!memcmp(data,"H1RD1\n",6)&&(data[6]=='0'||data[6]=='1')&&data[7]=='\n'&&!token_valid(data+8,n-8,error,sizeof(error))){
  memcpy(api_token,data+8,n-8);app.rd_configured=n>8;app.rd_enabled=app.rd_configured&&data[6]=='1';
 }memset(data,0,sizeof(data));
}
static int change_config(const char *token,size_t n,int enabled,char *error,size_t cap){
 lock(&app.mu);if(app.busy||app.direct_busy||app.update.busy||app.update.task>=0){unlock(&app.mu);snprintf(error,cap,"Aguarda ou pausa a tarefa antes de mudar o Real-Debrid.");return -1;}
 if(enabled&&!n){unlock(&app.mu);snprintf(error,cap,"Introduz a API Real-Debrid antes de ativar.");return -1;}
 if(save_config(token,n,enabled)){unlock(&app.mu);snprintf(error,cap,"Nao foi possivel guardar a configuracao Real-Debrid na consola.");return -1;}
 if(strlen(api_token)!=n||memcmp(api_token,token,n)){job[0]=job_hash[0]=0;memset(file_jobs,0,sizeof(file_jobs));}
 /* token may point to api_token when only switching the saved preference. */
 if(token!=api_token){memset(api_token,0,sizeof(api_token));if(n)memcpy(api_token,token,n);}
 app.rd_enabled=enabled;app.rd_configured=n!=0;app.rd_active=0;unlock(&app.mu);return 0;
}
int rd_configure(const char *token,size_t n,char *error,size_t cap){if(token_valid(token,n,error,cap))return -1;return change_config(token,n,n!=0,error,cap);}
int rd_enable(int enabled,char *error,size_t cap){return change_config(api_token,strlen(api_token),enabled,error,cap);}
int rd_forget(char *error,size_t cap){return change_config("",0,0,error,cap);}
static int api(const char *route,const char *method,const void *body,size_t n,const char *type,RDJson *d,char *error,size_t cap){
 memset(d,0,sizeof(*d));char url[256];if(snprintf(url,sizeof(url),RD_BASE "%s",route)>=(int)sizeof(url))return -1;Reply r={malloc(512*1024+1),0};if(!r.p){snprintf(error,cap,"Sem memoria para a resposta Real-Debrid.");return -1;}int status=0;r.p[0]=0;
 int rc=rd_http(url,method,api_token,body,n,type,512*1024,collect,&r,&status,error,cap);
 rd_trace("api-response",status,rc);
 if(!rc&&(status<200||status>=300)){
  if(status==401)snprintf(error,cap,"API Real-Debrid invalida ou expirada. Introduz uma API valida.");
  else if(status==403)snprintf(error,cap,"Real-Debrid recusou o acesso. Confirma a conta Premium e as permissoes.");
  else if(status==429)snprintf(error,cap,"Limite de pedidos Real-Debrid atingido. Aguarda e volta a tentar.");
  else snprintf(error,cap,"Real-Debrid respondeu HTTP %d. Confirma o estado da conta e tenta novamente.",status);rc=-1;
 }
 if(!rc&&r.n&&rd_json_parse(d,r.p,r.n)){snprintf(error,cap,"Resposta Real-Debrid invalida ou demasiado complexa.");rc=-1;}
 if(rc||!r.n)free(r.p);/* A parsed document owns its original response until dispose. */
 return rc;
}
static void dispose(RDJson *d){if(d->v){free((void*)d->s);rd_json_free(d);}}
static int string(const RDJson *d,const char *key,char *out,size_t cap){return rd_json_string(d,rd_json_key(d,0,key),out,cap);}
static int number(const RDJson *d,int object,const char *key,uint64_t *n){return rd_json_uint(d,rd_json_key(d,object,key),n);}
static int form(const char *key,const char *value,char **out,size_t *len){
 size_t n=strlen(value),prefix=strlen(key);if(n>=MAGNET_CAP)return -1;char *s=malloc(prefix+3*n+2);if(!s)return -1;memcpy(s,key,prefix);size_t used=prefix;s[used++]='=';const char *hex="0123456789ABCDEF";
 for(size_t i=0;i<n;i++){unsigned char c=value[i];if(isalnum(c)||strchr("-_.~",c))s[used++]=(char)c;else{s[used++]='%';s[used++]=hex[c>>4];s[used++]=hex[c&15];}}s[used]=0;*out=s;*len=used;return 0;
}
static int send_form(const char *route,const char *key,const char *value,RDJson *d,char *error,size_t cap){char *body=NULL;size_t n;if(form(key,value,&body,&n)){snprintf(error,cap,"Parametro Real-Debrid demasiado longo.");return -1;}int rc=api(route,"POST",body,n,"application/x-www-form-urlencoded",d,error,cap);free(body);return rc;}
static int same_file(const char *path,const TFile *f){if(app.rd_magnet)return !strcmp(path[0]=='/'?path+1:path,f->name);size_t p=strlen(path),n=strlen(f->name);return p>=n&&!strcmp(path+p-n,f->name)&&(p==n||path[p-n-1]=='/');}
static int pkg_path(const char *path){size_t n=strlen(path);return n>=5&&!strcasecmp(path+n-4,".pkg");}
int rd_magnet_parse(const char *url,size_t n,char *error,size_t cap){
 /* Torrent contains ~46 KB of metadata. Keep it off the PS4 HTTP stack. */
 rd_trace("magnet-parse",(int)n,0);Magnet *m=calloc(1,sizeof(*m));if(!m){snprintf(error,cap,"Sem memoria para o magnet.");return -1;}
 int rc=magnet_parse_service(m,url,n,error,cap);free(m);if(rc)return rc;
 memcpy(magnet_url,url,n);magnet_url[n]=0;job[0]=job_hash[0]=0;memset(file_jobs,0,sizeof(file_jobs));rd_trace("magnet-saved",0,0);return 0;
}
static int magnet_files(const RDJson *d,char *error,size_t cap){
 int files=rd_json_key(d,0,"files"),ignored=0;const char *reason="Lista de ficheiros Real-Debrid invalida. Envia real-debrid-debug.log.";Torrent *t=calloc(1,sizeof(*t));if(!t){snprintf(error,cap,"Sem memoria para os dados do magnet.");return -1;}
 memcpy(t->hash,app.torrent.hash,20);strcpy(t->hashhex,app.torrent.hashhex);strcpy(t->name,app.torrent.name);
 if(files<0||d->v[files].type!='['||d->v[files].count<1||d->v[files].count>512)goto bad;
 for(int i=files+1;i<d->v[files].next;i=d->v[i].next){char path[512];uint64_t bytes;
  if(rd_json_string(d,rd_json_key(d,i,"path"),path,sizeof(path)))goto bad;
  if(!pkg_path(path)){ignored++;continue;}
  reason="O PKG tem tamanho ou caminho invalido. Nao foi descarregado.";
  if(number(d,i,"bytes",&bytes)||bytes<PKG_HEADER_SIZE||bytes>INT64_MAX-t->total)goto bad;
  if(t->nfiles>=MAX_FILES){reason="Este magnet tem mais de 32 PKG. Divide a transferencia em torrents menores.";goto bad;}
  const char *name=path[0]=='/'?path+1:path;
  /* Paths are display metadata only: storage uses fixed fileNN.pkg names. */
  for(const char *p=name;*p;p++)if((unsigned char)*p<32||*p=='\\'||*p==':')goto bad;
  for(const char *p=name;*p;){const char *end=strchr(p,'/');size_t k=end?(size_t)(end-p):strlen(p);if(!k||(k==1&&p[0]=='.')||(k==2&&p[0]=='.'&&p[1]=='.'))goto bad;p=end?end+1:p+k;if(end&&!*p)goto bad;}
  for(int f=0;f<t->nfiles;f++)if(!strcmp(name,t->files[f].name)){reason="O Real-Debrid devolveu caminhos PKG repetidos. Envia real-debrid-debug.log.";goto bad;}
  TFile *f=&t->files[t->nfiles++];strcpy(f->name,name);f->size=bytes;f->offset=t->total;t->total+=bytes;
 }
 rd_trace("magnet-file-list",t->nfiles,ignored);
 if(!t->nfiles){reason="Este magnet nao tem PKG diretos na lista Real-Debrid. ZIP, RAR e partes precisam de descompactacao, ainda nao suportada.";goto bad;}
 char dir[600];snprintf(dir,sizeof(dir),"%s/%s",app.root,t->hashhex);make_dir(dir);
 lock(&app.mu);torrent_free(&app.torrent);free(app.complete);app.complete=NULL;app.torrent=*t;strcpy(app.dir,dir);app.loaded=1;app.magnet_pending=0;unlock(&app.mu);free(t);return 0;
bad:rd_trace("magnet-file-list-rejected",t->nfiles,ignored);free(t);snprintf(error,cap,"%s",reason);return -1;
}
static int verify_magnet_files(char *error,size_t cap){
 unsigned char *header=malloc(PKG_HEADER_SIZE);if(!header){snprintf(error,cap,"Sem memoria para verificar os PKG.");return -1;}
 for(int i=0;i<app.torrent.nfiles;i++){char path[700];RemotePkg pkg;file_path(i,path);FILE *f=fopen(path,"rb");if(!f){free(header);snprintf(error,cap,"PKG indisponivel para verificacao.");return -1;}
  size_t n=fread(header,1,PKG_HEADER_SIZE,f);int bad=ferror(f);fseeko(f,0,SEEK_END);int64_t size=ftello(f);fclose(f);
  if(stopped()||bad||pkg_header_read(header,n,&pkg,error,cap)||size<0||(uint64_t)size!=app.torrent.files[i].size||pkg.size!=(uint64_t)size){free(header);if(!*error)snprintf(error,cap,"Tamanho do PKG nao corresponde ao magnet. Nao foi instalado.");return -1;}
 }free(header);return 0;
}
static int selection(const RDJson *d,char ids[512],char *error,size_t cap){
 int files=rd_json_key(d,0,"files"),seen[MAX_FILES]={0};size_t used=0;if(files<0||d->v[files].type!='['||d->v[files].count<1||d->v[files].count>512||(!app.rd_magnet&&d->v[files].count!=app.torrent.nfiles))goto bad;
 for(int i=files+1;i<d->v[files].next;i=d->v[i].next){char path[1024];uint64_t id,bytes;if(rd_json_string(d,rd_json_key(d,i,"path"),path,sizeof(path))||number(d,i,"id",&id)||!id||id>INT32_MAX||number(d,i,"bytes",&bytes))goto bad;int match=-1;
  /* Validate all IDs, including ignored entries, without a large stack array. */
  for(int k=files+1;k<i;k=d->v[k].next){uint64_t prior;if(number(d,k,"id",&prior)||prior==id)goto bad;}
  if(app.rd_magnet&&!pkg_path(path))continue;
  for(int f=0;f<app.torrent.nfiles;f++)if(same_file(path,&app.torrent.files[f])&&bytes==app.torrent.files[f].size){if(match>=0)goto bad;match=f;}
  if(match<0||seen[match]++)goto bad;int written=snprintf(ids+used,512-used,"%s%llu",used?",":"",(unsigned long long)id);if(written<0||(size_t)written>=512-used)goto bad;used+=written;
 }
 for(int i=0;i<app.torrent.nfiles;i++)if(!seen[i])goto bad;return 0;
bad:snprintf(error,cap,"Os ficheiros Real-Debrid nao correspondem ao torrent. Aceita apenas PKG diretos, sem arquivos ou partes.");return -1;
}
typedef struct {FILE *f;uint64_t n,expected,before;} FileSink;
static int write_file(const void *p,size_t n,void *ctx){FileSink *s=ctx;if(stopped()||n>s->expected-s->n||fwrite(p,1,n,s->f)!=n)return -1;s->n+=n;lock(&app.mu);app.done=s->before+s->n;unlock(&app.mu);return 0;}
static int download_files(const RDJson *d,int only,uint64_t before,char *error,size_t cap){
 int links=rd_json_key(d,0,"links"),seen[MAX_FILES]={0},expected=only<0?app.torrent.nfiles:1;
 rd_trace("download-links",expected,links>=0?d->v[links].count:-1);
 if(links<0||d->v[links].type!='['||d->v[links].count!=expected){snprintf(error,cap,"Real-Debrid devolveu %d links para %d PKG selecionados. Se continuarem a ser partes/arquivos, envia real-debrid-debug.log.",links>=0?d->v[links].count:0,expected);return -1;}
 for(int i=links+1;i<d->v[links].next&&!stopped();i=d->v[i].next){char link[4096],download[4096],name[512];RDJson u;uint64_t bytes=0;
  if(rd_json_string(d,i,link,sizeof(link))||send_form("unrestrict/link","link",link,&u,error,cap))return -1;
  int invalid=string(&u,"download",download,sizeof(download))||string(&u,"filename",name,sizeof(name))||number(&u,0,"filesize",&bytes);dispose(&u);if(invalid){snprintf(error,cap,"O Real-Debrid nao devolveu um link PKG direto valido.");return -1;}
  int match=-1;for(int f=0;f<app.torrent.nfiles;f++){if(only>=0&&f!=only)continue;const char *base=strrchr(app.torrent.files[f].name,'/');base=base?base+1:app.torrent.files[f].name;if(!strcmp(base,name)&&app.torrent.files[f].size==bytes){if(match>=0){match=-2;break;}match=f;}}
  if(match<0||seen[match]++){snprintf(error,cap,"Nome ou tamanho do PKG Real-Debrid nao corresponde ao torrent. Nao foi instalado.");return -1;}
  char path[700],part[720],message[512];file_path(match,path);snprintf(part,sizeof(part),"%s.rd.part",path);snprintf(message,sizeof(message),"Real-Debrid: a guardar %.390s em /data/pkg.",name);set_status("downloading",message);
  FileSink sink={fopen(part,"wb"),0,bytes,before};if(!sink.f){snprintf(error,cap,"Nao foi possivel guardar o PKG Real-Debrid no disco.");return -1;}int status=0;
  int rc=rd_http(download,"GET",NULL,NULL,0,NULL,bytes,write_file,&sink,&status,error,cap);int closed=fclose(sink.f);
  if(rc||status!=200||sink.n!=bytes||closed){if(!*error)snprintf(error,cap,"Download Real-Debrid incompleto (HTTP %d). O PKG nao foi instalado.",status);return -1;}
#ifdef _WIN32
  if(!MoveFileExA(part,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
#else
  if(rename(part,path)){
#endif
   snprintf(error,cap,"Nao foi possivel concluir a gravacao do PKG Real-Debrid.");return -1;}
  before+=bytes;
 }
 return stopped()?-1:0;
}
static int new_job(char id[65],char *error,size_t cap){
 RDJson d;int rc;
 if(app.rd_magnet){set_status("rd-upload","Real-Debrid: a enviar o magnet diretamente, sem procurar peers.");rc=send_form("torrents/addMagnet","magnet",magnet_url,&d,error,cap);}
 else{
  char path[700];snprintf(path,sizeof(path),"%s/source.torrent",app.dir);FILE *f=fopen(path,"rb");if(!f){snprintf(error,cap,"Envia o ficheiro .torrent antes de usar Real-Debrid.");return -1;}
  fseeko(f,0,SEEK_END);int64_t len=ftello(f);fseeko(f,0,SEEK_SET);unsigned char *body=len>0&&len<=MAX_TORRENT?malloc((size_t)len):NULL;
  if(!body){fclose(f);snprintf(error,cap,"Torrent indisponivel ou demasiado grande.");return -1;}size_t got=fread(body,1,(size_t)len,f);fclose(f);
  set_status("rd-upload","Real-Debrid: a enviar o torrent.");rc=got!=(size_t)len?-1:api("torrents/addTorrent","PUT",body,(size_t)len,"application/x-bittorrent",&d,error,cap);free(body);
 }if(rc)return -1;
 rc=string(&d,"id",id,65);dispose(&d);if(rc||!id[0])goto bad;
 for(size_t i=0;id[i];i++)if(!isalnum((unsigned char)id[i]))goto bad;return 0;
bad:snprintf(error,cap,"Real-Debrid nao devolveu um identificador de torrent valido.");return -1;
}
static int one_file_id(const RDJson *d,int target,int require_selected,char id[32],char *error,size_t cap){
 char ids[512];if(selection(d,ids,error,cap))return -1;int files=rd_json_key(d,0,"files"),found=0;
 for(int i=files+1;i<d->v[files].next;i=d->v[i].next){char path[1024];uint64_t number_id,bytes,on=0;
  if(rd_json_string(d,rd_json_key(d,i,"path"),path,sizeof(path))||number(d,i,"id",&number_id)||number(d,i,"bytes",&bytes)||number(d,i,"selected",&on)||on>1)goto bad;
  int match=same_file(path,&app.torrent.files[target])&&bytes==app.torrent.files[target].size;
  if(require_selected&&(on!=(uint64_t)match))goto bad;
  if(match){snprintf(id,32,"%llu",(unsigned long long)number_id);found++;}
 }if(found==1)return 0;
bad:snprintf(error,cap,"Real-Debrid nao confirmou a selecao individual do PKG. Envia real-debrid-debug.log.");return -1;
}
static int pending_state(const char *state){return !strcmp(state,"magnet_conversion")||!strcmp(state,"queued")||!strcmp(state,"downloading")||!strcmp(state,"compressing")||!strcmp(state,"uploading");}
static void wait_poll(void){for(int i=0;i<50&&!stopped();i++)sleep_ms(100);}
static int download_individual(char *error,size_t cap){
 uint64_t before=0;if(!file_jobs[0][0])strcpy(file_jobs[0],job);
 for(int target=0;target<app.torrent.nfiles&&!stopped();target++){
  rd_trace("individual-file",target,app.torrent.nfiles);
  if(!file_jobs[target][0]&&new_job(file_jobs[target],error,cap))return -1;
  char route[128],state[64],id[32];snprintf(route,sizeof(route),"torrents/info/%s",file_jobs[target]);time_t deadline=time(NULL)+6*3600;int selected=0,ok=0;
  while(!stopped()&&time(NULL)<deadline){RDJson d;char hash[41];if(api(route,"GET",NULL,0,NULL,&d,error,cap))return -1;
   if(string(&d,"hash",hash,sizeof(hash))||strcasecmp(hash,app.torrent.hashhex)||string(&d,"status",state,sizeof(state))){dispose(&d);snprintf(error,cap,"Resposta Real-Debrid nao corresponde ao torrent enviado.");return -1;}
   if(!strcmp(state,"waiting_files_selection")){
    int rc=selected?-1:one_file_id(&d,target,0,id,error,cap);dispose(&d);if(rc)return -1;
    char select_route[128];snprintf(select_route,sizeof(select_route),"torrents/selectFiles/%s",file_jobs[target]);rd_trace("select-one",target,1);
    if(send_form(select_route,"files",id,&d,error,cap))return -1;dispose(&d);selected=1;
   }else if(!strcmp(state,"downloaded")){
    int rc=one_file_id(&d,target,1,id,error,cap);if(!rc)rc=download_files(&d,target,before,error,cap);dispose(&d);if(rc)return -1;ok=1;break;
   }else{int pending=pending_state(state);dispose(&d);if(!pending){snprintf(error,cap,"Real-Debrid nao conseguiu preparar o PKG %d (%s).",target+1,state);return -1;}}
   set_status("rd-waiting","Real-Debrid: a preparar cada PKG separadamente. Mantem a app aberta.");wait_poll();
  }
  if(!ok){if(!stopped())snprintf(error,cap,"O Real-Debrid ainda nao preparou o PKG. Retoma mais tarde.");return -1;}before+=app.torrent.files[target].size;
 }return stopped()?-1:0;
}
static void *worker(int prepare){
 char error[512]="",route[128],state[64];RDJson d;int ok=0;rd_trace("worker",prepare,app.torrent.nfiles);set_status("rd-account","Real-Debrid: a verificar a conta Premium.");
 if(api("user","GET",NULL,0,NULL,&d,error,sizeof(error)))goto done;
 char type[32];uint64_t premium=0;int valid=!string(&d,"type",type,sizeof(type))&&!strcmp(type,"premium")&&!number(&d,0,"premium",&premium)&&premium>0;dispose(&d);if(!valid){snprintf(error,sizeof(error),"Precisas de uma conta Real-Debrid Premium ativa para descarregar torrents.");goto done;}
 if(storage_check(app.root,app.torrent.total+64*1024*1024ULL,error,sizeof(error)))goto done;
 if(!job[0]||strcmp(job_hash,app.torrent.hashhex)){
  memset(file_jobs,0,sizeof(file_jobs));if(new_job(job,error,sizeof(error)))goto done;strcpy(job_hash,app.torrent.hashhex);
 }
 if(!prepare&&app.loaded&&app.torrent.nfiles>1){if(download_individual(error,sizeof(error)))goto done;goto verify;}
 snprintf(route,sizeof(route),"torrents/info/%s",job);time_t deadline=time(NULL)+6*3600;int selected=0;
 while(!stopped()&&time(NULL)<deadline){
  if(api(route,"GET",NULL,0,NULL,&d,error,sizeof(error)))goto done;char hash[41];if(string(&d,"hash",hash,sizeof(hash))||strcasecmp(hash,app.torrent.hashhex)||string(&d,"status",state,sizeof(state))){dispose(&d);snprintf(error,sizeof(error),"Resposta Real-Debrid nao corresponde ao torrent enviado.");goto done;}
  if(app.rd_magnet&&!app.loaded&&(!strcmp(state,"waiting_files_selection")||!strcmp(state,"downloaded"))){if(magnet_files(&d,error,sizeof(error))){dispose(&d);goto done;}}
  if(!prepare&&app.loaded&&app.torrent.nfiles>1){dispose(&d);if(download_individual(error,sizeof(error)))goto done;goto verify;}
  if(prepare&&app.loaded){char ids[512];int bad=selection(&d,ids,error,sizeof(error));dispose(&d);if(bad)goto done;ok=2;break;}
  if(app.loaded&&storage_check(app.root,app.torrent.total+64*1024*1024ULL,error,sizeof(error))){dispose(&d);goto done;}
  if(!strcmp(state,"waiting_files_selection")){char ids[512],select_route[128];int rc=selected?-1:selection(&d,ids,error,sizeof(error));dispose(&d);if(rc){if(!*error)snprintf(error,sizeof(error),"Real-Debrid nao confirmou a selecao dos PKG.");goto done;}snprintf(select_route,sizeof(select_route),"torrents/selectFiles/%s",job);if(send_form(select_route,"files",ids,&d,error,sizeof(error)))goto done;dispose(&d);selected=1;
  }else if(!strcmp(state,"downloaded")){char id[32];if(one_file_id(&d,0,1,id,error,sizeof(error))){dispose(&d);goto done;}int rc=download_files(&d,-1,0,error,sizeof(error));dispose(&d);if(rc)goto done;goto verify;
  }else{int pending=!strcmp(state,"magnet_conversion")||!strcmp(state,"queued")||!strcmp(state,"downloading")||!strcmp(state,"compressing")||!strcmp(state,"uploading");dispose(&d);if(!pending){snprintf(error,sizeof(error),"O Real-Debrid nao conseguiu preparar o torrent (%s).",state);goto done;}}
  set_status("rd-waiting","Real-Debrid: a preparar o torrent nos servidores. Podes pausar; a tarefa na conta mantem-se.");
  for(int i=0;i<50&&!stopped();i++)sleep_ms(100);
 }
 if(!ok&&!stopped())snprintf(error,sizeof(error),"O Real-Debrid ainda nao concluiu o torrent. Retoma mais tarde para consultar a mesma tarefa.");
 goto done;
verify:
 set_status("checking",app.rd_magnet?"Real-Debrid: a verificar tamanho e cabecalho dos PKG do magnet.":"Real-Debrid: a verificar todos os blocos do torrent antes de instalar.");
 if(app.rd_magnet?verify_magnet_files(error,sizeof(error)):torrent_verify_download(error,sizeof(error)))goto done;ok=1;
done:
 if(stopped()){lock(&app.mu);app.done=0;unlock(&app.mu);set_status("paused","Real-Debrid em pausa. Retomar consulta a mesma tarefa nesta sessao. A API e a preferencia ficam guardadas na consola.");}
 else if(ok==2)set_status("ready","Magnet preparado pelo Real-Debrid. Escolhe descarregar ou descarregar e instalar. Mantem a app aberta.");
 else if(ok){if(app.auto_install)torrent_install_all();else set_status("downloaded","Real-Debrid: PKG descarregados e verificados. Podes instalar.");}
 else{lock(&app.mu);app.done=0;unlock(&app.mu);set_status("error",*error?error:"Nao foi possivel concluir o download Real-Debrid.");}
 lock(&app.mu);app.busy=0;app.rd_active=0;unlock(&app.mu);return NULL;
}
void *rd_download_worker(void *unused){(void)unused;return worker(0);}
void *rd_prepare_magnet(void *unused){(void)unused;return worker(1);}
