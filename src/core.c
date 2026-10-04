#include "core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
uint32_t be32(const void *v){const unsigned char *p=v;return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
void put32(void *v,uint32_t n){unsigned char *p=v;p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
void put64(void *v,uint64_t n){put32(v,(uint32_t)(n>>32));put32((char*)v+4,(uint32_t)n);}
static uint32_t rol(uint32_t x,int n){return (x<<n)|(x>>(32-n));}
static void sha_block(uint32_t h[5],const unsigned char *p){
 uint32_t w[80],a=h[0],b=h[1],c=h[2],d=h[3],e=h[4];
 for(int i=0;i<16;i++)w[i]=be32(p+i*4);
 for(int i=16;i<80;i++)w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
 for(int i=0;i<80;i++){uint32_t f,k;
  if(i<20){f=(b&c)|(~b&d);k=0x5a827999;}else if(i<40){f=b^c^d;k=0x6ed9eba1;}
  else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8f1bbcdc;}else{f=b^c^d;k=0xca62c1d6;}
  uint32_t z=rol(a,5)+f+e+k+w[i];e=d;d=c;c=rol(b,30);b=a;a=z;
 }h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;
}
void sha1(const void *v,size_t n,unsigned char out[20]){
 const unsigned char *p=v;size_t length=n;uint32_t h[]={0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0};
 while(n>=64){sha_block(h,p);p+=64;n-=64;}
 unsigned char tail[128]={0};memcpy(tail,p,n);tail[n]=0x80;size_t k=n<56?64:128;
 put64(tail+k-8,(uint64_t)length*8);sha_block(h,tail);if(k==128)sha_block(h,tail+64);
 for(int i=0;i<5;i++)put32(out+i*4,h[i]);
}
static int node(BDoc *d,int depth){
 if(depth>24||d->pos>=d->size||d->count>=65536)return -1;
 if(d->count==d->cap){int c=d->cap?d->cap*2:64;BNode *q=realloc(d->nodes,c*sizeof(BNode));if(!q)return -1;d->nodes=q;d->cap=c;}
 int id=d->count++;BNode t={0};t.data=d->data;t.start=d->pos;unsigned char c=d->data[d->pos++];t.kind=c;
 if(c=='i'){
  int neg=0;if(d->pos<d->size&&d->data[d->pos]=='-'){neg=1;d->pos++;}
  size_t s=d->pos;uint64_t v=0;
  while(d->pos<d->size&&d->data[d->pos]>='0'&&d->data[d->pos]<='9'){
   unsigned z=d->data[d->pos++]-'0';if(v>((uint64_t)INT64_MAX-z)/10)return -1;v=v*10+z;
  }
  if(s==d->pos||d->pos>=d->size||d->data[d->pos++]!='e'||(d->pos-s>2&&d->data[s]=='0')||(neg&&v==0))return -1;
  t.number=neg?-(int64_t)v:(int64_t)v;
 }else if(c=='l'||c=='d'){
  int items=0,prev=-1;
  while(d->pos<d->size&&d->data[d->pos]!='e'){
   int ch=node(d,depth+1);if(ch<0)return -1;
   if(c=='d'&&items%2==0){
    if(d->nodes[ch].kind!='s')return -1;
    if(prev>=0){BNode *a=&d->nodes[prev],*b=&d->nodes[ch];size_t m=a->len<b->len?a->len:b->len;int cmp=memcmp(d->data+a->body,d->data+b->body,m);if(cmp>0||(cmp==0&&a->len>=b->len))return -1;}
    prev=ch;
   }items++;
  }
  if(d->pos>=d->size||(c=='d'&&items%2))return -1;d->pos++;
 }else if(c>='0'&&c<='9'){
  size_t n=c-'0';while(d->pos<d->size&&d->data[d->pos]>='0'&&d->data[d->pos]<='9'){
   if(c=='0'||n>d->size/10)return -1;n=n*10+d->data[d->pos++]-'0';
  }
  if(d->pos>=d->size||d->data[d->pos++]!=':'||n>d->size-d->pos)return -1;
  t.kind='s';t.body=d->pos;t.len=n;d->pos+=n;
 }else return -1;
 t.end=d->pos;t.next=d->count;d->nodes[id]=t;return id;
}
int bparse_prefix(BDoc *d,const unsigned char *p,size_t n,size_t *used){memset(d,0,sizeof(*d));d->data=p;d->size=n;if(!n||node(d,0)<0){bfree(d);return -1;}*used=d->pos;return 0;}
int bparse(BDoc *d,const unsigned char *p,size_t n){size_t used;if(bparse_prefix(d,p,n,&used))return -1;if(used!=n){bfree(d);return -1;}return 0;}
void bfree(BDoc *d){free(d->nodes);d->nodes=NULL;d->count=0;}
int bget(BDoc *d,int id,const char *key){
 if(id<0||id>=d->count||d->nodes[id].kind!='d')return -1;
 for(int k=id+1;k<d->nodes[id].next;){int v=k+1;BNode *a=&d->nodes[k];if(a->len==strlen(key)&&!memcmp(d->data+a->body,key,a->len))return v;k=d->nodes[v].next;}return -1;
}
int bstr(BDoc *d,int id,char *out,size_t cap){if(id<0||id>=d->count||d->nodes[id].kind!='s'||d->nodes[id].len>=cap)return -1;BNode *a=&d->nodes[id];if(memchr(d->data+a->body,0,a->len))return -1;memcpy(out,d->data+a->body,a->len);out[a->len]=0;return 0;}
static int64_t number(BDoc *d,int id){return id<0||d->nodes[id].kind!='i'?-1:d->nodes[id].number;}
static void tracker(Torrent *t,BDoc *d,int id){
 char s[1024];if(t->ntrackers>=MAX_TRACKERS||bstr(d,id,s,sizeof(s)))return;
 if(strncmp(s,"http://",7)&&strncmp(s,"udp://",6))return;
 for(size_t j=0;s[j];j++)if((unsigned char)s[j]<33||s[j]==127)return;
 for(int j=0;j<t->ntrackers;j++)if(!strcmp(t->trackers[j],s))return;
 strcpy(t->trackers[t->ntrackers++],s);
}
static int filename_ok(const char *s){size_t n=strlen(s);if(n<5||strcmp(s+n-4,".pkg"))return 0;for(size_t i=0;i<n;i++)if((unsigned char)s[i]<32)return 0;return 1;}
int torrent_parse(Torrent *t,const unsigned char *p,size_t n,char *err,size_t cap){
 BDoc d;memset(t,0,sizeof(*t));const char *why="Torrent invalido ou demasiado complexo.";
 if(n>MAX_TORRENT||bparse(&d,p,n)){snprintf(err,cap,"%s",why);return -1;}
 int info=bget(&d,0,"info");if(info<0)goto bad;
 int name=bget(&d,info,"name.utf-8");if(name<0)name=bget(&d,info,"name");if(bstr(&d,name,t->name,sizeof(t->name)))goto bad;
 int64_t pl=number(&d,bget(&d,info,"piece length"));if(pl<16384||pl>MAX_PIECE)goto bad;t->piece_size=(uint32_t)pl;
 int hashes=bget(&d,info,"pieces");if(hashes<0||d.nodes[hashes].kind!='s'||!d.nodes[hashes].len||d.nodes[hashes].len%20)goto bad;
 t->pieces=d.nodes[hashes].len/20;
 int fs=bget(&d,info,"files");
 if(fs<0){t->nfiles=1;snprintf(t->files[0].name,sizeof(t->files[0].name),"%s",t->name);int64_t sz=number(&d,bget(&d,info,"length"));if(sz<=0)goto bad;t->files[0].size=(uint64_t)sz;}
 else {
  if(d.nodes[fs].kind!='l')goto bad;
  for(int x=fs+1;x<d.nodes[fs].next;x=d.nodes[x].next){
   if(t->nfiles==MAX_FILES){why="Maximo de 32 ficheiros por torrent.";goto bad;}
   TFile *f=&t->files[t->nfiles++];int64_t sz=number(&d,bget(&d,x,"length"));if(sz<=0)goto bad;f->size=(uint64_t)sz;
   int path=bget(&d,x,"path.utf-8");if(path<0)path=bget(&d,x,"path");if(path<0||d.nodes[path].kind!='l')goto bad;
   for(int k=path+1;k<d.nodes[path].next;k=d.nodes[k].next){char part[256];if(bstr(&d,k,part,sizeof(part))||!part[0]||!strcmp(part,".")||!strcmp(part,"..")||strchr(part,'/')||strchr(part,'\\'))goto bad;
    size_t used=strlen(f->name),len=strlen(part);if(used+len+2>sizeof(f->name))goto bad;if(used)strcat(f->name,"/");strcat(f->name,part);
   }
  }
 }
 if(!t->nfiles)goto bad;
 for(int i=0;i<t->nfiles;i++){
  TFile *f=&t->files[i];if(!filename_ok(f->name)){why="Esta versao aceita apenas torrents com ficheiros .pkg diretos.";goto bad;}
  if(f->size>16ULL*1024*1024*1024*1024||t->total>16ULL*1024*1024*1024*1024-f->size)goto bad;
  f->offset=t->total;t->total+=f->size;
 }
 if((t->total+t->piece_size-1)/t->piece_size!=t->pieces)goto bad;
 tracker(t,&d,bget(&d,0,"announce"));int al=bget(&d,0,"announce-list");
 if(al>=0&&d.nodes[al].kind=='l')for(int x=al+1;x<d.nodes[al].next;x++)if(d.nodes[x].kind=='s')tracker(t,&d,x);
 int sources=bget(&d,0,"h1pNoise-peers");
 if(sources>=0){if(d.nodes[sources].kind!='l')goto bad;for(int x=sources+1;x<d.nodes[sources].next;x=d.nodes[x].next){
  if(t->nsources==MAX_SOURCES)goto bad;char host[256];int64_t port=number(&d,bget(&d,x,"port"));if(bstr(&d,bget(&d,x,"host"),host,sizeof(host))||!host[0]||port<1||port>65535)goto bad;
  for(size_t i=0;host[i];i++)if(!((host[i]>='a'&&host[i]<='z')||(host[i]>='A'&&host[i]<='Z')||(host[i]>='0'&&host[i]<='9')||host[i]=='.'||host[i]=='-'))goto bad;
  strcpy(t->sources[t->nsources].host,host);t->sources[t->nsources++].port=(int)port;
 }}
 if(!t->ntrackers&&!t->nsources){why="Sem tracker HTTP/UDP ou fonte direta suportada. HTTPS e DHT ainda nao estao implementados.";goto bad;}
 sha1(p+d.nodes[info].start,d.nodes[info].end-d.nodes[info].start,t->hash);
 for(int i=0;i<20;i++)sprintf(t->hashhex+2*i,"%02x",t->hash[i]);
 t->hashes=malloc(d.nodes[hashes].len);if(!t->hashes)goto bad;memcpy(t->hashes,p+d.nodes[hashes].body,d.nodes[hashes].len);bfree(&d);return 0;
bad:bfree(&d);torrent_free(t);snprintf(err,cap,"%s",why);return -1;
}
void torrent_free(Torrent *t){free(t->hashes);t->hashes=NULL;}
size_t jsonstr(char *out,size_t cap,const char *s){size_t n=0;if(cap<3)return 0;out[n++]='"';for(;*s&&n+7<cap;s++){unsigned char c=*s;if(c=='"'||c=='\\'){out[n++]='\\';out[n++]=c;}else if(c<32){snprintf(out+n,7,"\\u%04x",c);n+=6;}else out[n++]=c;}out[n++]='"';out[n]=0;return n;}
