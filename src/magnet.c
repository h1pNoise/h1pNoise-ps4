#include "magnet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int hex(unsigned char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static int decode(char *out,size_t cap,const char *p,size_t n){
 size_t k=0;for(size_t i=0;i<n;i++){unsigned char c=p[i];if(c=='%'){if(i+2>=n)return -1;int a=hex(p[i+1]),b=hex(p[i+2]);if(a<0||b<0)return -1;c=(a<<4)|b;i+=2;}if(c<32||c==127||k+1>=cap)return -1;out[k++]=c;}out[k]=0;return 0;
}
static int info_hash(const char *s,unsigned char h[20]){
 size_t n=strlen(s);if(n==40){for(int i=0;i<20;i++){int a=hex(s[2*i]),b=hex(s[2*i+1]);if(a<0||b<0)return -1;h[i]=(a<<4)|b;}return 0;}
 if(n!=32)return -1;unsigned bits=0,value=0,k=0;for(size_t i=0;i<n;i++){unsigned char c=s[i];if(c>='a'&&c<='z')c-=32;int v=c>='A'&&c<='Z'?c-'A':c>='2'&&c<='7'?c-'2'+26:-1;if(v<0)return -1;value=(value<<5)|v;bits+=5;if(bits>=8){bits-=8;h[k++]=(value>>bits)&255;}}return k==20?0:-1;
}
static int tracker_ok(const char *s){
 if(strncmp(s,"http://",7)&&strncmp(s,"udp://",6))return 0;
 const char *a=strstr(s,"://")+3,*e=strchr(a,'/');if(!e)e=a+strlen(a);if(a==e||memchr(a,'@',e-a)||memchr(a,'[',e-a))return 0;
 for(size_t i=0;s[i];i++)if((unsigned char)s[i]<=32||(unsigned char)s[i]>=127)return 0;return 1;
}
static int direct_peer(Magnet *m,const char *s){
 const char *colon=strrchr(s,':');if(!colon||colon==s||colon-s>=256)return -1;
 for(const char *p=s;p<colon;p++)if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='.'||*p=='-'))return -1;
 unsigned port=0;if(!colon[1])return -1;for(const char *p=colon+1;*p;p++){if(*p<'0'||*p>'9'||port>6553)return -1;port=port*10+*p-'0';}if(!port||port>65535)return -1;
 Torrent *t=&m->torrent;if(t->nsources==MAX_SOURCES)return -1;memcpy(t->sources[t->nsources].host,s,colon-s);t->sources[t->nsources].host[colon-s]=0;t->sources[t->nsources++].port=port;return 0;
}
static int parse(Magnet *m,const char *url,size_t n,char *error,size_t cap,int service){
 const char *why="Magnet invalido. Usa magnet:?xt=urn:btih:...";int found=0;memset(m,0,sizeof(*m));
 if(n<9||n>=MAGNET_CAP||memcmp(url,"magnet:?",8)||memchr(url,0,n)||memchr(url,'#',n))goto bad;
 for(size_t pos=8;pos<n;){size_t end=pos;while(end<n&&url[end]!='&')end++;const char *eq=memchr(url+pos,'=',end-pos);if(!eq)goto bad;
  size_t kn=eq-(url+pos);char key[64],value[2048];if(decode(key,sizeof(key),url+pos,kn)||decode(value,sizeof(value),eq+1,end-(size_t)(eq-url)-1))goto bad;
  if(!strcmp(key,"xt")&&!strncmp(value,"urn:btih:",9)){unsigned char h[20];if(info_hash(value+9,h))goto bad;if(found&&memcmp(h,m->torrent.hash,20)){why="O magnet contem identificadores de torrents diferentes.";goto bad;}memcpy(m->torrent.hash,h,20);found=1;}
  else if(!strcmp(key,"dn")){if(strlen(value)>=sizeof(m->torrent.name))goto bad;strcpy(m->torrent.name,value);}
  else if(!strcmp(key,"tr")&&tracker_ok(value)){if(strlen(value)>=sizeof(m->torrent.trackers[0]))goto bad;int duplicate=0;for(int i=0;i<m->torrent.ntrackers;i++)if(!strcmp(value,m->torrent.trackers[i]))duplicate=1;if(!duplicate){if(m->torrent.ntrackers==MAX_TRACKERS){why="O magnet tem demasiados trackers.";goto bad;}strcpy(m->torrent.trackers[m->torrent.ntrackers++],value);}}
  else if(!strcmp(key,"x.pe")&&direct_peer(m,value)){why="Fonte direta invalida. Usa um endereco IPv4 ou nome e porta.";goto bad;}
  pos=end+1;
 }
 if(!found){why="Este magnet precisa de um hash BitTorrent v1 (urn:btih). Magnets apenas v2 ainda nao sao suportados.";goto bad;}
 if(!service&&!m->torrent.ntrackers&&!m->torrent.nsources){why="O magnet nao tem tracker HTTP/UDP nem fonte direta. Esta versao nao usa DHT; envia o .torrent ou um magnet com trackers.";goto bad;}
 for(int i=0;i<20;i++)sprintf(m->torrent.hashhex+i*2,"%02x",m->torrent.hash[i]);if(!m->torrent.name[0])strcpy(m->torrent.name,"Magnet: a obter dados do torrent");return 0;
bad:snprintf(error,cap,"%s",why);return -1;
}
int magnet_parse(Magnet *m,const char *url,size_t n,char *error,size_t cap){return parse(m,url,n,error,cap,0);}
int magnet_parse_service(Magnet *m,const char *url,size_t n,char *error,size_t cap){return parse(m,url,n,error,cap,1);}
/* Reconstruct the metainfo without changing a single byte of the signed-by-hash info dictionary. */
int magnet_torrent(const Torrent *m,const unsigned char *info,size_t n,unsigned char **out,size_t *size,char *error,size_t cap){
 unsigned char hash[20];*out=NULL;*size=0;if(!n||n>MAGNET_METADATA_MAX)goto bad;sha1(info,n,hash);if(memcmp(hash,m->hash,20)){snprintf(error,cap,"Os dados recebidos nao correspondem ao hash do magnet.");return -1;}
 BDoc doc;if(bparse(&doc,info,n))goto bad;int dict=doc.nodes[0].kind=='d';bfree(&doc);if(!dict)goto bad;
 unsigned char *buf=malloc(n+32768);if(!buf){snprintf(error,cap,"Sem memoria para os dados do magnet.");return -1;}size_t k=0;
 /* x.pe-only magnets keep a tracker-free metainfo. Its source peers are attached after parsing. */
 k+=sprintf((char*)buf+k,"d13:announce-listl");for(int i=0;i<m->ntrackers;i++){size_t len=strlen(m->trackers[i]);k+=sprintf((char*)buf+k,"l%zu:",len);memcpy(buf+k,m->trackers[i],len);k+=len;buf[k++]='e';}buf[k++]='e';
 if(m->nsources){k+=sprintf((char*)buf+k,"14:h1pNoise-peersl");for(int i=0;i<m->nsources;i++)k+=sprintf((char*)buf+k,"d4:host%zu:%s4:porti%dee",strlen(m->sources[i].host),m->sources[i].host,m->sources[i].port);buf[k++]='e';}
 k+=sprintf((char*)buf+k,"4:info");memcpy(buf+k,info,n);k+=n;buf[k++]='e';*out=buf;*size=k;return 0;
bad:snprintf(error,cap,"Dados do magnet invalidos ou demasiado grandes.");return -1;
}
