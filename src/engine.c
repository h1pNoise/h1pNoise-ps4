#include "app.h"
#include "magnet.h"
#include <stdarg.h>
App app;
typedef struct{char ip[16];int port;} Peer;
typedef struct{char host[256],path[1200];int port,udp;} Url;
static Peer peers[256];static int npeers;
void set_status(const char *phase,const char *message){lock(&app.mu);snprintf(app.phase,sizeof(app.phase),"%s",phase);snprintf(app.message,sizeof(app.message),"%s",message);unlock(&app.mu);}
static int paused(void){lock(&app.mu);int v=app.pause;unlock(&app.mu);return v;}
static int is_complete(void){lock(&app.mu);int v=app.done==app.torrent.total;unlock(&app.mu);return v;}
void file_path(int i,char out[700]){snprintf(out,700,"%s/file%02d.pkg",app.dir,i);}
int data_io(uint64_t off,void *buf,size_t size,int writing){
 unsigned char *p=buf;int result=0;lock(&app.io);
 for(int i=0;i<app.torrent.nfiles&&size;i++){
  TFile *f=&app.torrent.files[i];if(off>=f->offset+f->size)continue;if(off<f->offset){result=-1;break;}
  uint64_t at=off-f->offset,avail=f->size-at;size_t n=avail<size?(size_t)avail:size;char path[700];file_path(i,path);
  FILE *fp=fopen(path,writing?"r+b":"rb");if(!fp&&writing)fp=fopen(path,"w+b");
  if(!fp){result=-1;break;}if(fseeko(fp,(int64_t)at,SEEK_SET)){fclose(fp);result=-1;break;}
  size_t got=writing?fwrite(p,1,n,fp):fread(p,1,n,fp);int closed=fclose(fp);if(got!=n||closed){result=-1;break;}size-=n;off+=n;p+=n;
 }unlock(&app.io);return size?-1:result;
}
static int save_torrent(const unsigned char *p,size_t n,char *error,size_t cap){
 Torrent *t=calloc(1,sizeof(Torrent));if(!t){snprintf(error,cap,"Sem memoria.");return -1;}
 if(torrent_parse(t,p,n,error,cap)){free(t);return -1;}unsigned char *complete=calloc(t->pieces,1);if(!complete){torrent_free(t);free(t);return -1;}
 char dir[600],path[700];snprintf(dir,sizeof(dir),"%s/%s",app.root,t->hashhex);make_dir(dir);snprintf(path,sizeof(path),"%s/source.torrent",dir);
 FILE *f=fopen(path,"wb");if(!f){snprintf(error,cap,"Nao foi possivel guardar o torrent no disco.");free(complete);torrent_free(t);free(t);return -1;}
 size_t wr=fwrite(p,1,n,f);int closed=fclose(f);if(wr!=n||closed){snprintf(error,cap,"Erro a guardar o torrent.");free(complete);torrent_free(t);free(t);return -1;}
 lock(&app.mu);torrent_free(&app.torrent);free(app.complete);app.complete=complete;app.torrent=*t;app.loaded=1;app.magnet_pending=0;app.done=0;app.pause=0;app.install_done=app.install_total=0;strcpy(app.dir,dir);unlock(&app.mu);free(t);
 snprintf(path,sizeof(path),"%s/current.txt",app.root);f=fopen(path,"wb");if(f){fputs(app.torrent.hashhex,f);fclose(f);}
 set_status("ready","Torrent recebido. Pronto para verificar e descarregar.");return 0;
}
int import_torrent(const unsigned char *p,size_t n,char *error,size_t cap){
 lock(&app.mu);if(app.busy||app.direct_busy||app.update.busy||app.update.task>=0){unlock(&app.mu);snprintf(error,cap,"Pausa o download ou aguarda a verificacao do link antes de trocar de torrent.");return -1;}unlock(&app.mu);
 return save_torrent(p,n,error,cap);
}
static int url_parse(const char *s,Url *u){
 memset(u,0,sizeof(*u));if(!strncmp(s,"http://",7)){s+=7;u->port=80;}else if(!strncmp(s,"udp://",6)){s+=6;u->udp=1;u->port=80;}else return -1;
 const char *end=strchr(s,'/');if(!end)end=s+strlen(s);const char *colon=memchr(s,':',end-s);size_t n=(colon?colon:end)-s;if(!n||n>=sizeof(u->host)||memchr(s,'@',end-s))return -1;
 memcpy(u->host,s,n);u->host[n]=0;if(colon){char *tail;long p=strtol(colon+1,&tail,10);if(tail!=end||p<1||p>65535)return -1;u->port=(int)p;}
 snprintf(u->path,sizeof(u->path),"%s",*end?end:"/");return 0;
}
static void add_peer(const unsigned char *p){if(npeers==256)return;Peer q;snprintf(q.ip,sizeof(q.ip),"%u.%u.%u.%u",p[0],p[1],p[2],p[3]);q.port=p[4]*256+p[5];if(!q.port||p[0]==0||p[0]>=224)return;for(int i=0;i<npeers;i++)if(q.port==peers[i].port&&!strcmp(q.ip,peers[i].ip))return;peers[npeers++]=q;}
static void source_peers(void){for(int i=0;i<app.torrent.nsources&&!paused();i++){struct in_addr addr;if(!resolve4(app.torrent.sources[i].host,&addr)){unsigned char p[6];memcpy(p,&addr,4);p[4]=app.torrent.sources[i].port>>8;p[5]=app.torrent.sources[i].port;add_peer(p);}}}
static int http_body(unsigned char *p,size_t *size){
 unsigned char *end=NULL;for(size_t i=0;i+3<*size;i++)if(!memcmp(p+i,"\r\n\r\n",4)){end=p+i;break;}
 if(!end||end-p>16384||*size<12||memcmp(p,"HTTP/1.",7)||memcmp(p+9,"200",3))return -1;
 size_t head=(size_t)(end-p)+4;int chunked=0;
 for(size_t i=0;i+26<head;i++)if(!strncasecmp((char*)p+i,"transfer-encoding: chunked",26))chunked=1;
 size_t n=*size-head;memmove(p,p+head,n);if(!chunked){*size=n;return 0;}
 size_t in=0,out=0;
 while(in<n){size_t len=0,digits=0;while(in<n&&p[in]!='\r'&&p[in]!=';'){unsigned char c=p[in++];unsigned v=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:99;if(v==99||digits++>8||len>n/16)return -1;len=len*16+v;}
  if(!digits)return -1;while(in<n&&p[in]!='\r')in++;if(in+2>n||p[in+1]!='\n')return -1;in+=2;if(!len){*size=out;return 0;}if(len>n-in||in+len+2>n)return -1;memmove(p+out,p+in,len);out+=len;in+=len;if(p[in]!='\r'||p[in+1]!='\n')return -1;in+=2;
 }return -1;
}
static int tracker_http(Url *u){
 char ih[61],pid[61];for(int i=0;i<20;i++){sprintf(ih+i*3,"%%%02X",app.torrent.hash[i]);sprintf(pid+i*3,"%%%02X",app.peer_id[i]);}
 char req[2600];lock(&app.mu);uint64_t downloaded=app.done;unlock(&app.mu);
 snprintf(req,sizeof(req),"GET %s%cinfo_hash=%s&peer_id=%s&port=6881&uploaded=0&downloaded=%llu&left=%llu&compact=1&numwant=80&event=started HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: Harbor/0.1\r\nConnection: close\r\n\r\n",u->path,strchr(u->path,'?')?'&':'?',ih,pid,(unsigned long long)downloaded,(unsigned long long)(app.magnet_pending?1:app.torrent.total-downloaded),u->host,u->port);
 Sock s=tcp_connect(u->host,u->port,6);if(s==BADSOCK)return -1;unsigned char *buf=malloc(262145);if(!buf){sockclose(s);return -1;}size_t n=0;
 if(send_all(s,req,strlen(req))){free(buf);sockclose(s);return -1;}
 while(n<262144){int k=recv(s,(char*)buf+n,262144-(int)n,0);if(k==0)break;if(k<0){free(buf);sockclose(s);return -1;}n+=k;}sockclose(s);
 if(http_body(buf,&n)){free(buf);return -1;}BDoc d;if(bparse(&d,buf,n)){free(buf);return -1;}
 int ix=bget(&d,0,"peers");if(ix>=0){BNode *a=&d.nodes[ix];if(a->kind=='s'&&a->len%6==0){for(size_t i=0;i<a->len;i+=6)add_peer(buf+a->body+i);}
  else if(a->kind=='l'){for(int x=ix+1;x<a->next;x=d.nodes[x].next){char ip[32];int port=bget(&d,x,"port");struct in_addr addr;if(!bstr(&d,bget(&d,x,"ip"),ip,sizeof(ip))&&inet_pton(AF_INET,ip,&addr)==1&&port>=0&&d.nodes[port].kind=='i'&&d.nodes[port].number>0&&d.nodes[port].number<=65535){unsigned char peer[6];memcpy(peer,&addr,4);peer[4]=d.nodes[port].number>>8;peer[5]=d.nodes[port].number;add_peer(peer);}}}
 }int ok=ix>=0;bfree(&d);free(buf);return ok?0:-1;
}
static int tracker_udp(Url *u){
 Sock s=socket(AF_INET,SOCK_DGRAM,0);if(s==BADSOCK)return -1;sock_timeout(s,5);struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons(u->port);
 if(resolve4(u->host,&a.sin_addr)||connect(s,(struct sockaddr*)&a,sizeof(a))){sockclose(s);return -1;}
 unsigned char req[98]={0},res[4096],tx[4];if(random_bytes(tx,4)){sockclose(s);return -1;}put64(req,0x41727101980ULL);put32(req+8,0);memcpy(req+12,tx,4);
 if(send(s,(char*)req,16,0)!=16){sockclose(s);return -1;}int n=recv(s,(char*)res,sizeof(res),0);if(n<16||be32(res)!=0||memcmp(res+4,tx,4)){sockclose(s);return -1;}
 memcpy(req,res+8,8);put32(req+8,1);memcpy(req+12,tx,4);memcpy(req+16,app.torrent.hash,20);memcpy(req+36,app.peer_id,20);
 lock(&app.mu);uint64_t done=app.done;unlock(&app.mu);put64(req+56,done);put64(req+64,app.magnet_pending?1:app.torrent.total-done);put64(req+72,0);put32(req+80,2);put32(req+84,0);memcpy(req+88,tx,4);put32(req+92,100);req[96]=0x1a;req[97]=0xe1;
 if(send(s,(char*)req,98,0)!=98){sockclose(s);return -1;}n=recv(s,(char*)res,sizeof(res),0);sockclose(s);if(n<20||be32(res)!=1||memcmp(res+4,tx,4)||(n-20)%6)return -1;for(int i=20;i+6<=n;i+=6)add_peer(res+i);return 0;
}
static uint32_t piece_len(uint32_t i){uint64_t left=app.torrent.total-(uint64_t)i*app.torrent.piece_size;return left<app.torrent.piece_size?(uint32_t)left:app.torrent.piece_size;}
static int message(Sock s,unsigned char **buf,size_t *cap,uint32_t *n){unsigned char h[4];if(recv_all(s,h,4))return -1;*n=be32(h);if(*n>1048576)return -1;if(*n>*cap){void *p=realloc(*buf,*n);if(!p)return -1;*buf=p;*cap=*n;}return *n?recv_all(s,*buf,*n):0;}
static int claim(const unsigned char *have){int idx=-1;lock(&app.mu);for(uint32_t i=0;i<app.torrent.pieces;i++)if(have[i]&&!app.complete[i]){app.complete[i]=2;idx=(int)i;break;}unlock(&app.mu);return idx;}
static void release_piece(int piece){if(piece<0)return;lock(&app.mu);if(app.complete[piece]==2)app.complete[piece]=0;unlock(&app.mu);}
static void *peer_worker(void *arg){
 Peer p=*(Peer*)arg;Sock s=tcp_connect(p.ip,p.port,6);if(s==BADSOCK)return NULL;
 unsigned char handshake[68]={19};memcpy(handshake+1,"BitTorrent protocol",19);memcpy(handshake+28,app.torrent.hash,20);memcpy(handshake+48,app.peer_id,20);
 if(send_all(s,handshake,68)||recv_all(s,handshake,68)||handshake[0]!=19||memcmp(handshake+1,"BitTorrent protocol",19)||memcmp(handshake+28,app.torrent.hash,20)){sockclose(s);return NULL;}
 unsigned char interested[]={0,0,0,1,2};if(send_all(s,interested,sizeof(interested))){sockclose(s);return NULL;}
 unsigned char *have=calloc(app.torrent.pieces,1),*piece=malloc(app.torrent.piece_size),*msg=NULL;size_t mcap=0;
 if(!have||!piece){free(have);free(piece);sockclose(s);return NULL;}
 lock(&app.mu);app.peers++;unlock(&app.mu);int active=-1,choked=1;time_t last=time(NULL),connected=time(NULL);
 while(!paused()&&!is_complete()&&time(NULL)-last<35&&time(NULL)-connected<120){
  uint32_t n;active=choked?-1:claim(have);
  if(active<0){
  if(message(s,&msg,&mcap,&n))break;if(!n)continue;
  if(msg[0]==0)choked=1;else if(msg[0]==1)choked=0;
  else if(msg[0]==4&&n==5){uint32_t i=be32(msg+1);if(i<app.torrent.pieces)have[i]=1;}
  else if(msg[0]==5){size_t bytes=(app.torrent.pieces+7)/8;if(n!=bytes+1)break;for(uint32_t i=0;i<app.torrent.pieces;i++)have[i]=(msg[1+i/8]>>(7-i%8))&1;}
  continue;
  }
  uint32_t len=piece_len(active),blocks=(len+16383)/16384,sent=0,received=0;unsigned char seen[1024]={0};int failed=0;
  while(received<blocks&&!paused()){
   while(sent<blocks&&sent-received<8){uint32_t off=sent*16384,sz=len-off;if(sz>16384)sz=16384;unsigned char req[17];put32(req,13);req[4]=6;put32(req+5,active);put32(req+9,off);put32(req+13,sz);if(send_all(s,req,17)){failed=1;break;}sent++;}
   if(failed||message(s,&msg,&mcap,&n)){failed=1;break;}if(!n)continue;
   if(msg[0]==0){failed=1;break;}
   if(msg[0]==7){if(n<9||be32(msg+1)!=(uint32_t)active){failed=1;break;}uint32_t off=be32(msg+5);if(off>=len||off%16384||off/16384>=sent){failed=1;break;}uint32_t sz=len-off;if(sz>16384)sz=16384;if(n!=sz+9){failed=1;break;}
    if(!seen[off/16384]){memcpy(piece+off,msg+9,sz);seen[off/16384]=1;received++;last=time(NULL);}
   }else if(msg[0]==4&&n==5){uint32_t i=be32(msg+1);if(i<app.torrent.pieces)have[i]=1;}
   if(time(NULL)-last>35){failed=1;break;}
  }
  if(failed||paused()){release_piece(active);active=-1;break;}
  unsigned char hash[20];sha1(piece,len,hash);if(memcmp(hash,app.torrent.hashes+active*20,20)){release_piece(active);active=-1;break;}
  if(data_io((uint64_t)active*app.torrent.piece_size,piece,len,1)){set_status("error","Erro de escrita no disco. O download foi interrompido.");lock(&app.mu);app.pause=1;unlock(&app.mu);release_piece(active);active=-1;break;}
  lock(&app.mu);app.complete[active]=1;app.done+=len;unlock(&app.mu);active=-1;
 }
 release_piece(active);lock(&app.mu);app.peers--;unlock(&app.mu);free(have);free(piece);free(msg);sockclose(s);return NULL;
}
static void install_progress(uint64_t n,uint64_t total){lock(&app.mu);app.install_done=n;app.install_total=total;unlock(&app.mu);}
static int install_all(void){
 /* Read package flags, never infer package order only from filenames. */
 int order[MAX_FILES],patch[MAX_FILES];char paths[MAX_FILES][700];
 for(int i=0;i<app.torrent.nfiles;i++){
  order[i]=i;file_path(i,paths[i]);FILE *f=fopen(paths[i],"rb");unsigned char h[128];if(!f){set_status("error","PKG nao encontrado.");return -1;}size_t n=fread(h,1,sizeof(h),f);fclose(f);
  if(n!=sizeof(h)||memcmp(h,"\x7f" "CNT",4)){set_status("error","O ficheiro descarregado nao tem um cabecalho PKG valido.");return -1;}
  uint32_t flags=be32(h+0x78);patch[i]=(flags&(0x00100000u|0x40000000u|0x41000000u|0x60000000u))!=0;
 }
 for(int i=0;i<app.torrent.nfiles;i++)for(int j=i+1;j<app.torrent.nfiles;j++)if(patch[order[i]]>patch[order[j]]){int x=order[i];order[i]=order[j];order[j]=x;}
 for(int i=0;i<app.torrent.nfiles;i++){
  int f=order[i];char msg[512],error[512];snprintf(msg,sizeof(msg),"A instalar %d/%d: %.400s",i+1,app.torrent.nfiles,app.torrent.files[f].name);set_status("installing",msg);
  if(install_pkg(paths[f],app.torrent.files[f].name,error,sizeof(error),install_progress)){set_status("error",error);return -1;}
 }
 set_status("installed","Instalacao concluida. Os PKG foram mantidos no disco.");return 0;
}
static void finished(void){lock(&app.mu);app.busy=0;unlock(&app.mu);}
/* BEP 10 uses our local ID for incoming messages and the peer's ID for requests. */
static int metadata_peer(const Peer *peer,unsigned char **info,size_t *size,time_t deadline){
 Sock s=tcp_connect(peer->ip,peer->port,6);if(s==BADSOCK)return -1;int rc=-1;
 unsigned char handshake[68]={19},*msg=NULL,*metadata=NULL;size_t mcap=0;uint32_t n=0;
 memcpy(handshake+1,"BitTorrent protocol",19);handshake[25]=0x10;memcpy(handshake+28,app.torrent.hash,20);memcpy(handshake+48,app.peer_id,20);
 if(send_all(s,handshake,68)||recv_all(s,handshake,68)||handshake[0]!=19||memcmp(handshake+1,"BitTorrent protocol",19)||memcmp(handshake+28,app.torrent.hash,20)||!(handshake[25]&0x10))goto end;
 const char *hello="d1:md11:ut_metadatai1eee";unsigned char header[6];put32(header,(uint32_t)strlen(hello)+2);header[4]=20;header[5]=0;
 if(send_all(s,header,6)||send_all(s,hello,strlen(hello)))goto end;
 int remote_id=0;uint32_t total=0;time_t peer_deadline=time(NULL)+40;if(peer_deadline>deadline)peer_deadline=deadline;
 for(int received=0;received<128&&!paused()&&time(NULL)<peer_deadline;received++){
  if(message(s,&msg,&mcap,&n))goto end;if(n<2||msg[0]!=20||msg[1]!=0)continue;
  BDoc d;if(bparse(&d,msg+2,n-2))goto end;int id=bget(&d,bget(&d,0,"m"),"ut_metadata"),len=bget(&d,0,"metadata_size");
  if(id>=0&&len>=0&&d.nodes[id].kind=='i'&&d.nodes[len].kind=='i'&&d.nodes[id].number>0&&d.nodes[id].number<=255&&d.nodes[len].number>0&&d.nodes[len].number<=MAGNET_METADATA_MAX){remote_id=(int)d.nodes[id].number;total=(uint32_t)d.nodes[len].number;}bfree(&d);break;
 }
 if(!remote_id||!total||paused())goto end;metadata=malloc(total);if(!metadata)goto end;
 lock(&app.mu);app.peers=1;unlock(&app.mu);
 for(uint32_t piece=0;piece<(total+16383)/16384&&!paused();piece++){
  char req[96];int rn=snprintf(req,sizeof(req),"d8:msg_typei0e5:piecei%uee",piece);put32(header,rn+2);header[5]=(unsigned char)remote_id;
  if(send_all(s,header,6)||send_all(s,req,rn))goto end;
  int got=0;for(int received=0;received<128&&!paused()&&time(NULL)<peer_deadline;received++){
   if(message(s,&msg,&mcap,&n))goto end;if(n<2||msg[0]!=20||msg[1]!=1)continue;
   BDoc d;size_t used;if(bparse_prefix(&d,msg+2,n-2,&used))goto end;
   int type=bget(&d,0,"msg_type"),index=bget(&d,0,"piece"),len=bget(&d,0,"total_size");
   if(type<0||index<0||d.nodes[type].kind!='i'||d.nodes[index].kind!='i'){bfree(&d);goto end;}
   int64_t kind=d.nodes[type].number,part=d.nodes[index].number;
   if(kind==2){bfree(&d);goto end;}if(kind!=1){bfree(&d);continue;}
   uint32_t bytes=total-piece*16384;if(bytes>16384)bytes=16384;
   if(part!=piece||len<0||d.nodes[len].kind!='i'||d.nodes[len].number!=total||n-2-used!=bytes){bfree(&d);goto end;}
   memcpy(metadata+piece*16384,msg+2+used,bytes);bfree(&d);got=1;break;
  }if(!got)goto end;
 }
 if(!paused()){unsigned char hash[20];sha1(metadata,total,hash);if(!memcmp(hash,app.torrent.hash,20)){*info=metadata;*size=total;metadata=NULL;rc=0;}}
end:lock(&app.mu);app.peers=0;unlock(&app.mu);free(metadata);free(msg);sockclose(s);return rc;
}
static void *magnet_worker(void *unused){
 (void)unused;npeers=0;time_t deadline=time(NULL)+180;set_status("metadata","A procurar fontes para obter os dados do magnet.");source_peers();
 for(int i=0;i<app.torrent.ntrackers&&!paused()&&time(NULL)<deadline;i++){Url u;if(!url_parse(app.torrent.trackers[i],&u)){if(u.udp)tracker_udp(&u);else tracker_http(&u);}}
 unsigned char *info=NULL,*metainfo=NULL;size_t size=0,meta_size=0;int ok=0;char error[512]="Nao foi possivel obter os dados do magnet. Confirma fontes disponiveis e tenta de novo ou envia o .torrent.";
 for(int i=0;i<npeers&&!paused()&&time(NULL)<deadline;i++){
  char progress[256];snprintf(progress,sizeof(progress),"A obter os dados do magnet: fonte %d de %d. Podes cancelar.",i+1,npeers);set_status("metadata",progress);
  if(!metadata_peer(&peers[i],&info,&size,deadline)){ok=1;break;}
 }
 if(paused())set_status("paused","Procura do magnet cancelada. Envia o magnet novamente para tentar de novo.");
 else if(ok&&!magnet_torrent(&app.torrent,info,size,&metainfo,&meta_size,error,sizeof(error))&&!save_torrent(metainfo,meta_size,error,sizeof(error)))set_status("ready","Magnet recebido e verificado. Escolhe descarregar ou descarregar e instalar.");
 else set_status("error",error);
 free(info);free(metainfo);finished();return NULL;
}
int begin_magnet(const char *url,size_t n,char *error,size_t cap){
 Magnet *m=calloc(1,sizeof(*m));if(!m){snprintf(error,cap,"Sem memoria.");return -1;}if(magnet_parse(m,url,n,error,cap)){free(m);return -1;}
 lock(&app.mu);if(app.busy||app.direct_busy||app.update.busy||app.update.task>=0){unlock(&app.mu);free(m);snprintf(error,cap,"Pausa a tarefa atual antes de enviar outro magnet.");return -1;}
 torrent_free(&app.torrent);free(app.complete);app.complete=NULL;app.torrent=m->torrent;app.loaded=0;app.magnet_pending=1;app.busy=1;app.pause=0;app.done=0;app.peers=0;app.install_done=app.install_total=0;app.direct_phase[0]=0;app.direct_message[0]=0;snprintf(app.phase,sizeof(app.phase),"metadata");snprintf(app.message,sizeof(app.message),"A procurar os dados do magnet. Mantem a app aberta.");unlock(&app.mu);free(m);
 Thread t;if(thread_start(&t,magnet_worker,NULL)){set_status("error","Nao foi possivel iniciar a procura do magnet.");finished();snprintf(error,cap,"Nao foi possivel iniciar a procura do magnet.");return -1;}
#ifdef _WIN32
 CloseHandle(t);
#else
 pthread_detach(t);
#endif
 return 0;
}
static void *download_worker(void *unused){
 (void)unused;set_status("checking","A verificar os blocos existentes para retomar o download.");unsigned char *buf=malloc(app.torrent.piece_size);if(!buf){set_status("error","Sem memoria para verificar o torrent.");finished();return NULL;}
 lock(&app.mu);app.done=0;memset(app.complete,0,app.torrent.pieces);unlock(&app.mu);
 for(uint32_t i=0;i<app.torrent.pieces&&!paused();i++){uint32_t n=piece_len(i);if(!data_io((uint64_t)i*app.torrent.piece_size,buf,n,0)){unsigned char h[20];sha1(buf,n,h);if(!memcmp(h,app.torrent.hashes+i*20,20)){lock(&app.mu);app.complete[i]=1;app.done+=n;unlock(&app.mu);}}}free(buf);
 char space_error[512];
 if(!paused()&&storage_check(app.root,app.torrent.total-app.done+64*1024*1024ULL,space_error,sizeof(space_error))){set_status("error",space_error);finished();return NULL;}
 while(!paused()&&!is_complete()){
  npeers=0;set_status("trackers","A procurar peers nos trackers do torrent.");source_peers();
  for(int i=0;i<app.torrent.ntrackers&&!paused();i++){Url u;if(!url_parse(app.torrent.trackers[i],&u)){if(u.udp)tracker_udp(&u);else tracker_http(&u);}}
  if(!npeers){set_status("waiting","Sem peers acessiveis. Nova tentativa dentro de 60 segundos. Esta versao nao usa DHT.");for(int i=0;i<60&&!paused();i++)sleep_ms(1000);continue;}
  set_status("downloading","A descarregar e verificar blocos. Mantem a aplicacao aberta.");
  for(int p=0;p<npeers&&!paused()&&!is_complete();p+=4){Thread threads[4];int started[4]={0};for(int j=0;j<4&&p+j<npeers;j++)started[j]=!thread_start(&threads[j],peer_worker,&peers[p+j]);for(int j=0;j<4;j++)if(started[j])thread_join(threads[j]);}
  if(!paused()&&!is_complete()){set_status("waiting","A aguardar peers com blocos disponiveis. Nova tentativa dentro de 60 segundos.");for(int i=0;i<60&&!paused();i++)sleep_ms(1000);}
 }
 if(paused()){lock(&app.mu);int error=!strcmp(app.phase,"error");unlock(&app.mu);if(!error)set_status("paused","Download em pausa. Os blocos verificados ficam guardados.");}
 else if(app.auto_install)install_all();else set_status("downloaded","Download completo e verificado. Podes iniciar a instalacao.");finished();return NULL;
}
int begin_download(int install,char *error,size_t cap){
 lock(&app.mu);if(!app.loaded||app.busy||app.direct_busy||app.update.busy||app.update.task>=0){unlock(&app.mu);snprintf(error,cap,"Importa um torrent ou aguarda pela operacao atual.");return -1;}
 app.busy=1;app.pause=0;app.auto_install=install;unlock(&app.mu);Thread t;
 if(thread_start(&t,download_worker,NULL)){finished();snprintf(error,cap,"Nao foi possivel iniciar o download.");return -1;}
#ifdef _WIN32
 CloseHandle(t);
#else
 pthread_detach(t);
#endif
 return 0;
}
static void *install_worker(void *unused){(void)unused;install_all();finished();return NULL;}
int begin_install(char *error,size_t cap){
 lock(&app.mu);if(!app.loaded||app.busy||app.direct_busy||app.update.busy||app.update.task>=0||app.done!=app.torrent.total){unlock(&app.mu);snprintf(error,cap,"O download tem de estar completo e verificado. Aguarda qualquer verificacao de link.");return -1;}app.busy=1;unlock(&app.mu);Thread t;
 if(thread_start(&t,install_worker,NULL)){finished();return -1;}
#ifdef _WIN32
 CloseHandle(t);
#else
 pthread_detach(t);
#endif
 return 0;
}
