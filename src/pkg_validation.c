#include "remote_pkg.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
static int reject(char *e,size_t cap,const char *s){snprintf(e,cap,"%s",s);return -1;}
int pkg_url_valid(const char *url,size_t len,char *error,size_t cap){
 if(!url||!len||len>=PKG_URL_CAP)return reject(error,cap,"Cola um link direto HTTP ou HTTPS com menos de 2048 caracteres.");
 for(size_t i=0;i<len;i++)if((unsigned char)url[i]<=32||(unsigned char)url[i]>=127||url[i]=='\\'||url[i]=='#')return reject(error,cap,"O link tem espacos ou caracteres invalidos. Usa o endereco direto, com os espacos codificados.");
 size_t start=0;if(len>7&&!memcmp(url,"http://",7))start=7;else if(len>8&&!memcmp(url,"https://",8))start=8;
 if(!start)return reject(error,cap,"Usa um link direto que comece por http:// ou https://.");
 size_t end=start;while(end<len&&url[end]!='/'&&url[end]!='?')end++;
 size_t host_end=start;while(host_end<end&&url[host_end]!=':')host_end++;
 if(host_end==start||host_end-start>253)return reject(error,cap,"O servidor do link e invalido.");
 for(size_t i=start;i<host_end;i++)if(!isalnum((unsigned char)url[i])&&url[i]!='.'&&url[i]!='-')return reject(error,cap,"Usa um link sem login no endereco, com dominio ou IPv4.");
 if(host_end<end){unsigned port=0;if(host_end+1==end)return reject(error,cap,"Porta invalida no link.");for(size_t i=host_end+1;i<end;i++){if(url[i]<'0'||url[i]>'9'||port>6553)return reject(error,cap,"Porta invalida no link.");port=port*10+url[i]-'0';}if(!port||port>65535)return reject(error,cap,"Porta invalida no link.");}
 for(size_t i=end;i<len;i++)if(url[i]=='%'){if(i+2>=len||!isxdigit((unsigned char)url[i+1])||!isxdigit((unsigned char)url[i+2]))return reject(error,cap,"Codificacao invalida no link.");i+=2;}
 return 0;
}
int pkg_bgft_url(const char *url,char *out,size_t out_cap,char *error,size_t cap){
 if(out&&out_cap)out[0]=0;
 if(!url)return reject(error,cap,"O link PKG e invalido.");
 size_t len=strlen(url);
 if(pkg_url_valid(url,len,error,cap))return -1;
 /* BGFT checks the end of the URL, even when a query follows the PKG path.
    A fragment supplies the extension without changing the HTTP path/query or
    invalidating signed links. Keep the original URL for header verification.
    Reference: ItsJokerZz/FPKGi issue 12, comment 2700065495. */
 const char suffix[]="#content.pkg";
 size_t extra=(len>=4&&(!memcmp(url+len-4,".pkg",4)||!memcmp(url+len-4,".PKG",4)))?0:sizeof(suffix)-1;
 if(!out||len+extra>=out_cap||len+extra>=PKG_URL_CAP)return reject(error,cap,"O link e demasiado longo para enviar ao sistema da PS4. Usa um link direto mais curto.");
 memcpy(out,url,len);
 if(extra)memcpy(out+len,suffix,extra);
 out[len+extra]=0;return 0;
}
static uint32_t read32(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int pkg_header_read(const unsigned char *data,size_t len,RemotePkg *pkg,char *error,size_t cap){
 memset(pkg,0,sizeof(*pkg));
 if(len<PKG_HEADER_SIZE||memcmp(data,"\x7f" "CNT",4))return reject(error,cap,"O link nao devolveu um PKG completo. Copia o link do ficheiro, nao o da pagina.");
 memcpy(pkg->content_id,data+0x40,36);
 for(int i=0;i<36;i++){unsigned char c=pkg->content_id[i];if(((i==6||i==19)&&c!='-')||(i==16&&c!='_')||((i!=6&&i!=16&&i!=19)&&!isalnum(c)&&c!='_'))return reject(error,cap,"O Content ID do PKG e invalido.");}
 if(data[0x64])return reject(error,cap,"O Content ID do PKG e demasiado longo.");
 memcpy(pkg->title_id,pkg->content_id+7,9);
 /* First implementation accepts complete applications and full updates only. */
 if(read32(data+0x74)!=0x1a)return reject(error,cap,"Este modo aceita jogos/aplicacoes e atualizacoes completas. DLC e delta PKG ainda nao sao suportados.");
 uint32_t flags=read32(data+0x78);if((flags&0x41000000u)==0x41000000u)return reject(error,cap,"Usa uma atualizacao completa. Delta PKG nao suportado neste modo.");pkg->patch=(flags&0x60100000u)!=0;
 pkg->size=((uint64_t)read32(data+0x430)<<32)|read32(data+0x434);
 if(pkg->size<PKG_HEADER_SIZE||pkg->size>INT64_MAX)return reject(error,cap,"Tamanho invalido no PKG.");
 return 0;
}
static int number(const char **p,const char *end,uint64_t *value){
 *value=0;const char *start=*p;while(*p<end&&**p>='0'&&**p<='9'){unsigned digit=(unsigned)(*(*p)++-'0');if(*value>(UINT64_MAX-digit)/10)return -1;*value=*value*10+digit;}return *p==start?-1:0;
}
int pkg_range_total(const char *h,size_t len,uint64_t *total){
 /* Require one unambiguous response to Range: bytes=0-8191. */
 if(!h||len>16384)return -1;const char *end=h+len;int found=0;*total=0;
 for(const char *line=h;line<end;){const char *next=memchr(line,'\n',(size_t)(end-line));if(!next)next=end;const char *p=line;
  const char key[]="content-range:";size_t k=0;while(k<sizeof(key)-1&&p+k<next&&tolower((unsigned char)p[k])==key[k])k++;
  if(k==sizeof(key)-1){if(found++)return -1;p+=k;while(p<next&&(*p==' '||*p=='\t'))p++;if(next-p<6||memcmp(p,"bytes ",6))return -1;p+=6;uint64_t first,last;
   if(number(&p,next,&first)||p==next||*p++!='-'||number(&p,next,&last)||p==next||*p++!='/'||number(&p,next,total))return -1;
   while(p<next&&(*p==' '||*p=='\t'||*p=='\r'||*p==0))p++;if(p!=next||first!=0||last!=PKG_HEADER_SIZE-1||*total<PKG_HEADER_SIZE)return -1;
  }line=next<end?next+1:end;
 }return found==1?0:-1;
}
