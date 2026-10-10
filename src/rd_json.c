#include "rd_json.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
static void space(RDJson *d){while(d->pos<d->n&&strchr(" \r\n\t",d->s[d->pos]))d->pos++;}
static int hex(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static int value(RDJson *d,int depth){
 space(d);if(depth>24||d->pos>=d->n||d->count==d->cap)return -1;
 int id=d->count++;RDNode *v=&d->v[id];v->start=d->pos;char c=d->s[d->pos++];v->type=c;
 if(c=='{'||c=='['){space(d);char end=c=='{'?'}':']';if(d->pos<d->n&&d->s[d->pos]==end)d->pos++;
  else for(;;){if(c=='{'){space(d);if(d->pos>=d->n||d->s[d->pos]!='"'||value(d,depth+1)<0)return -1;space(d);if(d->pos>=d->n||d->s[d->pos++]!=':')return -1;}
   if(value(d,depth+1)<0)return -1;v->count++;space(d);if(d->pos>=d->n)return -1;c=d->s[v->start];char sep=d->s[d->pos++];if(sep==end)break;if(sep!=',')return -1;}
 }else if(c=='"'){int closed=0;while(d->pos<d->n){unsigned char a=d->s[d->pos++];if(a=='"'){closed=1;break;}if(a<32)return -1;if(a=='\\'){if(d->pos>=d->n)return -1;a=d->s[d->pos++];if(a=='u'){if(d->n-d->pos<4)return -1;for(int i=0;i<4;i++)if(hex(d->s[d->pos++])<0)return -1;}else if(!strchr("\"\\/bfnrt",a))return -1;}}if(!closed)return -1;
 }else if(c=='t'||c=='f'||c=='n'){const char *literal=c=='t'?"true":c=='f'?"false":"null";size_t len=strlen(literal);if(d->n-v->start<len||memcmp(d->s+v->start,literal,len))return -1;d->pos=v->start+len;
 }else {v->type='i';size_t p=v->start;if(c=='-'){if(p+1>=d->n)return -1;p++;}if(d->s[p]=='0')p++;else{if(d->s[p]<'1'||d->s[p]>'9')return -1;while(p<d->n&&isdigit((unsigned char)d->s[p]))p++;}
  if(p<d->n&&d->s[p]=='.'){p++;size_t begin=p;while(p<d->n&&isdigit((unsigned char)d->s[p]))p++;if(p==begin)return -1;}
  if(p<d->n&&(d->s[p]=='e'||d->s[p]=='E')){p++;if(p<d->n&&(d->s[p]=='+'||d->s[p]=='-'))p++;size_t begin=p;while(p<d->n&&isdigit((unsigned char)d->s[p]))p++;if(p==begin)return -1;}d->pos=p;
 }
 v->end=d->pos;v->next=d->count;return id;
}
void rd_json_free(RDJson *d){free(d->v);d->v=NULL;}
int rd_json_parse(RDJson *d,const char *s,size_t n){memset(d,0,sizeof(*d));if(!n||n>512*1024||memchr(s,0,n))return -1;d->s=s;d->n=n;d->cap=8192;d->v=calloc(d->cap,sizeof(*d->v));if(!d->v)return -1;if(value(d,0)<0){rd_json_free(d);return -1;}space(d);if(d->pos!=n){rd_json_free(d);return -1;}return 0;}
static unsigned unicode(const char *s){unsigned u=0;for(int i=0;i<4;i++)u=u*16+(unsigned)hex(s[i]);return u;}
int rd_json_string(const RDJson *d,int id,char *out,size_t cap){
 if(!out||!cap)return -1;out[0]=0;if(id<0||id>=d->count||d->v[id].type!='"')return -1;size_t used=0;
 for(size_t p=d->v[id].start+1;p+1<d->v[id].end;){unsigned u=(unsigned char)d->s[p++];char bytes[4];size_t len=1;bytes[0]=(char)u;
  if(u=='\\'){u=(unsigned char)d->s[p++];if(u=='u'){u=unicode(d->s+p);p+=4;if(u>=0xd800&&u<=0xdbff){if(p+6>=d->v[id].end||d->s[p]!='\\'||d->s[p+1]!='u')return -1;unsigned low=unicode(d->s+p+2);if(low<0xdc00||low>0xdfff)return -1;u=0x10000+((u-0xd800)<<10)+low-0xdc00;p+=6;}else if(u>=0xdc00&&u<=0xdfff)return -1;
    if(u<128){bytes[0]=(char)u;}else if(u<2048){len=2;bytes[0]=0xc0|(u>>6);bytes[1]=0x80|(u&63);}else if(u<65536){len=3;bytes[0]=0xe0|(u>>12);bytes[1]=0x80|((u>>6)&63);bytes[2]=0x80|(u&63);}else{len=4;bytes[0]=0xf0|(u>>18);bytes[1]=0x80|((u>>12)&63);bytes[2]=0x80|((u>>6)&63);bytes[3]=0x80|(u&63);}
   }else{const char *esc="bfnrt",*found=strchr(esc,(int)u);bytes[0]=found?"\b\f\n\r\t"[found-esc]:(char)u;}}
  if(!u||used+len>=cap)return -1;memcpy(out+used,bytes,len);used+=len;
 }out[used]=0;return 0;
}
int rd_json_key(const RDJson *d,int node,const char *key){if(node<0||node>=d->count||d->v[node].type!='{')return -1;int found=-1;for(int i=node+1;i<d->v[node].next;){char k[128];if(rd_json_string(d,i,k,sizeof(k)))return -1;int v=i+1;if(!strcmp(k,key)){if(found>=0)return -1;found=v;}i=d->v[v].next;}return found;}
int rd_json_uint(const RDJson *d,int id,uint64_t *out){if(id<0||id>=d->count||d->v[id].type!='i')return -1;*out=0;RDNode *v=&d->v[id];for(size_t p=v->start;p<v->end;p++){unsigned char c=d->s[p];if(c<'0'||c>'9'||*out>(UINT64_MAX-(c-'0'))/10)return -1;*out=*out*10+c-'0';}return 0;}
