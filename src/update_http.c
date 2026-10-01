#include "app.h"
#include "updater.h"
#include "version.h"
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
#include <stdbool.h>
#include <orbis/Net.h>
#include <orbis/Sysmodule.h>
extern int32_t sceSslInit(size_t),sceSslTerm(int32_t);
extern int32_t sceHttpInit(int32_t,int32_t,size_t),sceHttpTerm(int32_t);
extern int32_t sceHttpCreateTemplate(int32_t,const char*,int32_t,int32_t),sceHttpDeleteTemplate(int32_t);
extern int32_t sceHttpSetConnectTimeOut(int32_t,uint32_t),sceHttpSetResolveTimeOut(int32_t,uint32_t),sceHttpSetRecvTimeOut(int32_t,uint32_t),sceHttpSetSendTimeOut(int32_t,uint32_t);
extern int32_t sceHttpSetAutoRedirect(int32_t,int32_t);
extern int32_t sceHttpSetResponseHeaderMaxSize(int32_t,uint64_t);
/* GitHub release redirects include long signed Location and security headers.
 * Configure libSceHttp before sending; parsing only after receipt is too late. */
#define UPDATE_RESPONSE_HEADER_MAX 32768u
extern int32_t sceHttpCreateConnectionWithURL(int32_t,const char*,bool),sceHttpDeleteConnection(int32_t);
extern int32_t sceHttpCreateRequestWithURL(int32_t,int32_t,const char*,uint64_t),sceHttpDeleteRequest(int32_t);
extern int32_t sceHttpAddRequestHeader(int32_t,const char*,const char*,int32_t),sceHttpSendRequest(int32_t,const void*,size_t);
extern int32_t sceHttpGetStatusCode(int32_t,int32_t*),sceHttpGetAllResponseHeaders(int32_t,char**,size_t*),sceHttpReadData(int32_t,void*,uint32_t);
static int location(const char *headers,size_t n,char out[2048]){
 int found=0;size_t p=0;while(p<n){size_t end=p;while(end<n&&headers[end]!='\n')end++;
  if(end-p>=9&&!strncasecmp(headers+p,"Location:",9)){size_t a=p+9,b=end;while(a<b&&(headers[a]==' '||headers[a]=='\t'))a++;while(b>a&&(headers[b-1]=='\r'||headers[b-1]==' '))b--;if(found++||b-a>=2048)return -1;memcpy(out,headers+a,b-a);out[b-a]=0;}
  p=end+1;
 }return found==1?0:-1;
}
static int request(const char *url,uint64_t limit,UpdateSink sink,void *context,char redirect[2048],char *error,size_t cap){
 int pool=-1,ssl=-1,http=-1,tmpl=-1,conn=-1,req=-1,rc=-1,result=-1,status=0;unsigned char buf[32768];uint64_t total=0;
 if((rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL))<0||(rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP))<0)goto done;
 if((pool=rc=sceNetPoolCreate("h1pNoise-update",512*1024,0))<0||(ssl=rc=sceSslInit(512*1024))<0||(http=rc=sceHttpInit(pool,ssl,1024*1024))<0)goto done;
 if((tmpl=rc=sceHttpCreateTemplate(http,"h1pNoise/" APP_VERSION,2,0))<0)goto done;
 if((rc=sceHttpSetResponseHeaderMaxSize(tmpl,UPDATE_RESPONSE_HEADER_MAX))<0||(rc=sceHttpSetAutoRedirect(tmpl,0))<0||(rc=sceHttpSetConnectTimeOut(tmpl,10000000))<0||(rc=sceHttpSetResolveTimeOut(tmpl,10000000))<0||(rc=sceHttpSetRecvTimeOut(tmpl,15000000))<0||(rc=sceHttpSetSendTimeOut(tmpl,10000000))<0)goto done;
 if((conn=rc=sceHttpCreateConnectionWithURL(tmpl,url,false))<0||(req=rc=sceHttpCreateRequestWithURL(conn,0,url,0))<0)goto done;
 if((rc=sceHttpAddRequestHeader(req,"Accept-Encoding","identity",0))<0||(rc=sceHttpSendRequest(req,NULL,0))<0||(rc=sceHttpGetStatusCode(req,&status))<0)goto done;
 if(status==301||status==302||status==303||status==307||status==308){char *headers=NULL;size_t n=0;if((rc=sceHttpGetAllResponseHeaders(req,&headers,&n))<0)goto done;if(n>UPDATE_RESPONSE_HEADER_MAX){rc=(int32_t)0x80431073u;goto done;}if(!headers||location(headers,n,redirect)){snprintf(error,cap,"O servidor devolveu um redirecionamento de atualizacao invalido.");goto done;}result=1;goto done;}
 if(status!=200){snprintf(error,cap,status==404?"Ainda nao existe uma versao publicada no GitHub (HTTP 404).":"O servidor de atualizacoes respondeu HTTP %d.",status);goto done;}
 time_t start=time(NULL);for(;;){if(time(NULL)-start>600){snprintf(error,cap,"O download da atualizacao excedeu o tempo limite.");goto done;}
  rc=sceHttpReadData(req,buf,sizeof(buf));if(rc<0)goto done;if(!rc)break;if((size_t)rc>sizeof(buf)||(uint64_t)rc>limit-total){snprintf(error,cap,"A atualizacao ultrapassou o tamanho permitido.");goto done;}
  total+=(size_t)rc;if(sink(buf,(size_t)rc,context)){snprintf(error,cap,"Nao foi possivel guardar ou validar a atualizacao.");goto done;}}
 result=0;
done:
 if(result<0&&!*error){
  if((uint32_t)rc==0x80431073u)snprintf(error,cap,"A resposta do servidor excedeu o limite de cabecalhos da atualizacao (0x80431073).");
  else if((uint32_t)rc==0x80431075u)snprintf(error,cap,"A ligacao HTTPS da atualizacao falhou (0x80431075). Confirma a data da consola e o certificado do servidor.");
  else snprintf(error,cap,"Nao foi possivel obter a atualizacao (0x%08X). Confirma a ligacao da consola.",(unsigned)rc);
 }
 if(req>=0)sceHttpDeleteRequest(req);if(conn>=0)sceHttpDeleteConnection(conn);if(tmpl>=0)sceHttpDeleteTemplate(tmpl);if(http>=0)sceHttpTerm(http);if(ssl>=0)sceSslTerm(ssl);if(pool>=0)sceNetPoolDestroy(pool);return result;
}
int update_http_get(const char *url,uint64_t limit,UpdateSink sink,void *context,char *error,size_t cap){
 if(!update_https_url(url)){snprintf(error,cap,"Endereco HTTPS de atualizacao invalido.");return -1;}
 char current[2048],next[2048];strcpy(current,url);for(int hop=0;hop<6;hop++){
  next[0]=0;int rc=request(current,limit,sink,context,next,error,cap);if(rc<=0)return rc;
  if(next[0]=='/'&&next[1]!='/'){char full[2048];const char *end=strchr(current+8,'/');size_t host_len=end?(size_t)(end-current):strlen(current);if(host_len+strlen(next)>=sizeof(full))break;memcpy(full,current,host_len);strcpy(full+host_len,next);strcpy(next,full);}
  if(!update_redirect_allowed(url,next)){snprintf(error,cap,"O servidor redirecionou para um endereco de atualizacao nao autorizado.");return -1;}strcpy(current,next);
 }snprintf(error,cap,"Demasiados redirecionamentos na atualizacao.");return -1;
}
#else
int update_http_get(const char *u,uint64_t limit,UpdateSink sink,void *ctx,char *e,size_t cap){(void)u;(void)limit;(void)sink;(void)ctx;snprintf(e,cap,"As atualizacoes da aplicacao requerem uma PS4 real. Esta e uma versao de teste.");return -1;}
#endif
