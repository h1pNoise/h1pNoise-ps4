#include "update_manifest.h"
#include "version.h"
#include "vendor/monocypher-ed25519.h"
#include <string.h>
#include <stdio.h>
static int bad(char *e,size_t n,const char *s){snprintf(e,n,"%s",s);return -1;}
static int number(const char *s,uint64_t max,uint64_t *out){uint64_t n=0;if(!*s)return -1;for(;*s;s++){unsigned c=(unsigned char)*s-'0';if(c>9||n>(max-c)/10)return -1;n=n*10+c;}*out=n;return 0;}
static int hex(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
static int host(const char *s,char out[254]){
 if(strncmp(s,"https://",8))return -1;s+=8;int n=0;
 while(*s&&*s!='/'&&*s!='?'){char c=*s++;if(n==253||!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='-'))return -1;out[n++]=c;}
 out[n]=0;return n?0:-1;
}
int update_https_url(const char *s){char h[254];size_t n=strlen(s);if(n<9||n>=2048||host(s,h))return 0;for(size_t i=0;i<n;i++){unsigned char c=s[i];if(c<=32||c>=127||c=='\\'||c=='#')return 0;if(c=='%'&&(i+2>=n||hex(s[i+1]|32)<0||hex(s[i+2]|32)<0))return 0;}return 1;}
int update_redirect_allowed(const char *initial,const char *next){
 char a[254],b[254];if(!update_https_url(initial)||!update_https_url(next)||host(initial,a)||host(next,b))return 0;
 if(!strcmp(a,b))return 1;
 return !strcmp(a,"github.com")&&(!strcmp(b,"release-assets.githubusercontent.com")||!strcmp(b,"objects.githubusercontent.com"));
}
int update_manifest_read(const unsigned char *data,size_t n,const unsigned char key[32],UpdateManifest *out,char *error,size_t cap){
 memset(out,0,sizeof(*out));
 if(n<65||n>UPDATE_MANIFEST_MAX||crypto_ed25519_check(data,key,data+64,n-64))return bad(error,cap,"A assinatura da atualizacao nao e valida. Nada foi instalado.");
 char body[UPDATE_MANIFEST_MAX],*fields[9];size_t length=n-64;memcpy(body,data+64,length);body[length]=0;
 size_t start=0;int count=0;for(size_t i=0;i<length;i++){unsigned char c=body[i];if(!c||c=='\r'||(c<32&&c!='\n')||c==127)return bad(error,cap,"Metadados da atualizacao invalidos.");if(c=='\n'){if(count==9)return bad(error,cap,"Campos extra na atualizacao.");fields[count++]=body+start;body[i]=0;start=i+1;}}
#ifdef HARBOR_RUNTIME_UPDATES
 const char *format="H1PNOISE-PS4-RUNTIME-1";
#else
 const char *format="H1PNOISE-PS4-UPDATE-1";
#endif
 if(count!=9||start!=length||strcmp(fields[0],format)||strcmp(fields[1],APP_CONTENT_ID))return bad(error,cap,"Esta atualizacao nao pertence a h1pNoise para PS4.");
 size_t vlen=strlen(fields[2]);if(!vlen||vlen>=sizeof(out->version))return bad(error,cap,"Versao invalida.");
 for(size_t i=0;i<vlen;i++)if((fields[2][i]<'0'||fields[2][i]>'9')&&fields[2][i]!='.')return bad(error,cap,"Versao invalida.");
 int dots=0,digits=0;for(size_t i=0;i<vlen;i++){if(fields[2][i]=='.'){if(!digits||++dots>2)return bad(error,cap,"Versao invalida.");digits=0;}else digits++;}if(dots!=2||!digits)return bad(error,cap,"Versao invalida.");
 if(strlen(fields[3])!=5||fields[3][2]!='.'||fields[3][0]<'0'||fields[3][0]>'9'||fields[3][1]<'0'||fields[3][1]>'9'||fields[3][3]<'0'||fields[3][3]>'9'||fields[3][4]<'0'||fields[3][4]>'9')return bad(error,cap,"Versao PS4 invalida.");
 uint64_t build,size;if(number(fields[4],2147483647,&build)||!build||number(fields[5],UPDATE_PACKAGE_MAX,&size)||size<8192)return bad(error,cap,"Tamanho ou numero de versao invalido.");
 if(strlen(fields[6])!=128)return bad(error,cap,"Resumo do pacote invalido.");
 for(int i=0;i<64;i++){int a=hex(fields[6][2*i]),b=hex(fields[6][2*i+1]);if(a<0||b<0)return bad(error,cap,"Resumo do pacote invalido.");out->sha512[i]=(a<<4)|b;}
 if(!update_https_url(fields[7])||strlen(fields[8])>=sizeof(out->notes))return bad(error,cap,"Endereco ou notas da atualizacao invalidos.");
 out->build=(uint32_t)build;out->size=size;strcpy(out->version,fields[2]);strcpy(out->sfo,fields[3]);strcpy(out->url,fields[7]);strcpy(out->notes,fields[8]);return 0;
}
static uint32_t le(const unsigned char *p){return (uint32_t)p[3]<<24|(uint32_t)p[2]<<16|(uint32_t)p[1]<<8|p[0];}
#ifndef HARBOR_RUNTIME_UPDATES
static uint32_t be(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static unsigned le16(const unsigned char *p){return p[0]|p[1]<<8;}
static int field(const unsigned char *p,size_t n,const char *key,const char *value){
 if(n<20||le(p)!=0x46535000)return 0;uint32_t keys=le(p+8),vals=le(p+12),count=le(p+16);if(count>(n-20)/16||keys>n||vals>n)return 0;int found=0;
 for(uint32_t i=0;i<count;i++){const unsigned char *e=p+20+i*16;uint64_t k=(uint64_t)keys+le16(e),v=(uint64_t)vals+le(e+12);uint32_t length=le(e+4);if(k>=n||v>n||length>n-v||!memchr(p+k,0,n-k))return 0;
  if(!strcmp((const char*)p+k,key)){if(found++||le16(e+2)!=0x204||length!=strlen(value)+1||memcmp(p+v,value,length))return 0;}}
 return found==1;
}
#endif
int update_pkg_metadata(const unsigned char *p,size_t n,uint64_t size,const UpdateManifest *m){
#ifdef HARBOR_RUNTIME_UPDATES
 /* The signed manifest binds this executable to our title/version. No raw
    ELF or external title is accepted. SELF header file size is uint64 LE. */
 if(n<8192||size!=m->size||le(p)!=0x1d3d154f||p[6]!=1||p[7]!=0x12)return -1;
 uint64_t declared=(uint64_t)le(p+0x10)|((uint64_t)le(p+0x14)<<32);
 return declared==size?0:-1;
#else
 if(n<8192||size!=m->size||memcmp(p,"\x7f" "CNT",4)||memcmp(p+0x40,APP_CONTENT_ID,37)||be(p+0x74)!=0x1a)return -1;
 if(((uint64_t)be(p+0x430)<<32|be(p+0x434))!=size)return -1;
 uint32_t entries=be(p+0x10),table=be(p+0x18);if(table>n||entries>(n-table)/32)return -1;int found=0;
 for(uint32_t i=0;i<entries;i++){const unsigned char *e=p+table+i*32;if(be(e)!=0x1000)continue;if(found++)return -1;uint32_t off=be(e+16),len=be(e+20);if(off>n||len>n-off)return -1;
  if(!field(p+off,len,"TITLE_ID",APP_TITLE_ID)||!field(p+off,len,"CONTENT_ID",APP_CONTENT_ID)||!field(p+off,len,"APP_VER",m->sfo)||!field(p+off,len,"CATEGORY","gd"))return -1;}
 return found==1?0:-1;
#endif
}
