#include "real_debrid.h"
#include "app.h"
#include "version.h"
#include <ctype.h>
#if defined(_WIN32)
#include <winhttp.h>
#elif defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
#include <stdbool.h>
#include <orbis/Net.h>
#include <orbis/Sysmodule.h>
extern int32_t sceSslInit(size_t),sceSslTerm(int32_t);
extern int32_t sceHttpInit(int32_t,int32_t,size_t),sceHttpTerm(int32_t);
extern int32_t sceHttpCreateTemplate(int32_t,const char*,int32_t,int32_t),sceHttpDeleteTemplate(int32_t);
extern int32_t sceHttpSetConnectTimeOut(int32_t,uint32_t),sceHttpSetResolveTimeOut(int32_t,uint32_t),sceHttpSetRecvTimeOut(int32_t,uint32_t),sceHttpSetSendTimeOut(int32_t,uint32_t);
extern int32_t sceHttpSetAutoRedirect(int32_t,int32_t),sceHttpSetResponseHeaderMaxSize(int32_t,uint64_t);
extern int32_t sceHttpCreateConnectionWithURL(int32_t,const char*,bool),sceHttpDeleteConnection(int32_t);
extern int32_t sceHttpCreateRequestWithURL(int32_t,int32_t,const char*,uint64_t),sceHttpDeleteRequest(int32_t);
extern int32_t sceHttpAddRequestHeader(int32_t,const char*,const char*,int32_t),sceHttpSendRequest(int32_t,const void*,size_t);
extern int32_t sceHttpGetStatusCode(int32_t,int32_t*),sceHttpGetAllResponseHeaders(int32_t,char**,size_t*),sceHttpReadData(int32_t,void*,uint32_t);
#endif
static int cancelled(void){lock(&app.mu);int p=app.pause;unlock(&app.mu);return p;}
/* Do not accept credentials, control characters, local names or local IPs in
 * service-supplied download/redirect URLs. TLS verification stays enabled. */
static int url_ok(const char *url,char host[256],const char **path){
 size_t n=strlen(url);if(n<9||n>=4096||strncmp(url,"https://",8))return -1;
 for(size_t i=0;i<n;i++)if((unsigned char)url[i]<=32||(unsigned char)url[i]>=127||url[i]=='\\'||url[i]=='#')return -1;
 const char *end=strchr(url+8,'/');if(!end)return -1;size_t len=end-url-8;if(!len||len>=256)return -1;
 for(size_t i=8;i<(size_t)(end-url);i++)if(!isalnum((unsigned char)url[i])&&url[i]!='-'&&url[i]!='.')return -1;
 memcpy(host,url+8,len);host[len]=0;if(!strchr(host,'.'))return -1;struct in_addr a;if(resolve4(host,&a))return -1;uint32_t ip=ntohl(a.s_addr);
 if((ip>>24)==0||(ip>>24)==10||(ip>>24)==127||(ip>>24)>=224||(ip>>16)==0xa9fe||(ip>>16)==0xc0a8||(ip>>20)==0xac1||(ip>>22)==0x191)return -1;
 *path=end;return 0;
}
static int once(const char *url,const char *method,const char *token,const void *body,size_t n,const char *type,uint64_t limit,RDSink sink,void *ctx,int *status,char next[4096],char *error,size_t cap){
 char host[256];const char *path;int result=-1;if(url_ok(url,host,&path)){snprintf(error,cap,"Endereco Real-Debrid HTTPS invalido ou servidor inacessivel.");return -1;}
 if(token&&*token&&strncmp(url,"https://api.real-debrid.com/rest/1.0/",37)){snprintf(error,cap,"Destino da API Real-Debrid recusado.");return -1;}
 unsigned char *buf=malloc(65536);if(!buf){snprintf(error,cap,"Sem memoria para o Real-Debrid.");return -1;}uint64_t total=0;next[0]=0;*status=0;
#ifdef _WIN32
 HINTERNET session=NULL,conn=NULL,req=NULL;wchar_t whost[256],wpath[4096],verb[16],headers[1024];DWORD rc=0;
 MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,host,-1,whost,256);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wpath,4096);MultiByteToWideChar(CP_UTF8,0,method,-1,verb,16);
 session=WinHttpOpen(L"h1pNoise Real-Debrid",WINHTTP_ACCESS_TYPE_NO_PROXY,NULL,NULL,0);if(!session)goto done;
 WinHttpSetTimeouts(session,10000,10000,15000,30000);conn=WinHttpConnect(session,whost,443,0);if(!conn)goto done;
 req=WinHttpOpenRequest(conn,verb,wpath,NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);if(!req)goto done;
 DWORD disabled=WINHTTP_DISABLE_REDIRECTS; if(!WinHttpSetOption(req,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof(disabled)))goto done;
 char raw[1024];snprintf(raw,sizeof(raw),"Accept-Encoding: identity\r\nContent-Type: %s\r\n%s%s%s",type?type:"application/octet-stream",token&&*token?"Authorization: Bearer ":"",token&&*token?token:"",token&&*token?"\r\n":"");MultiByteToWideChar(CP_UTF8,0,raw,-1,headers,1024);
 if(!WinHttpSendRequest(req,headers,(DWORD)-1,(void*)body,(DWORD)n,(DWORD)n,0)||!WinHttpReceiveResponse(req,NULL))goto done;
 DWORD code=0,len=sizeof(code);if(!WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,NULL,&code,&len,NULL))goto done;*status=(int)code;
 if(code>=300&&code<400){wchar_t location[4096];len=sizeof(location);if(!WinHttpQueryHeaders(req,WINHTTP_QUERY_LOCATION,NULL,location,&len,NULL)||!WideCharToMultiByte(CP_UTF8,0,location,-1,next,4096,NULL,NULL))goto done;result=1;goto done;}
 for(;;){DWORD got=0;if(cancelled())goto done;if(!WinHttpReadData(req,buf,65536,&got))goto done;if(!got)break;if(got>limit-total||sink(buf,got,ctx)){snprintf(error,cap,"Download Real-Debrid interrompido, demasiado grande ou sem espaco.");goto done;}total+=got;}result=0;
done:rc=GetLastError();if(req)WinHttpCloseHandle(req);if(conn)WinHttpCloseHandle(conn);if(session)WinHttpCloseHandle(session);
#elif defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 int pool=-1,ssl=-1,http=-1,tmpl=-1,conn=-1,req=-1;int32_t rc=-1;
 if((rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL))<0||(rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP))<0)goto done;
 if((pool=rc=sceNetPoolCreate("h1pNoise-rd",512*1024,0))<0||(ssl=rc=sceSslInit(512*1024))<0||(http=rc=sceHttpInit(pool,ssl,1024*1024))<0)goto done;
 if((tmpl=rc=sceHttpCreateTemplate(http,"h1pNoise/" APP_VERSION,2,0))<0)goto done;
 if((rc=sceHttpSetResponseHeaderMaxSize(tmpl,32768))<0||(rc=sceHttpSetAutoRedirect(tmpl,0))<0||(rc=sceHttpSetConnectTimeOut(tmpl,10000000))<0||(rc=sceHttpSetResolveTimeOut(tmpl,10000000))<0||(rc=sceHttpSetRecvTimeOut(tmpl,30000000))<0||(rc=sceHttpSetSendTimeOut(tmpl,15000000))<0)goto done;
 int verb=!strcmp(method,"GET")?0:!strcmp(method,"POST")?1:!strcmp(method,"PUT")?4:-1;if(verb<0)goto done;
 if((conn=rc=sceHttpCreateConnectionWithURL(tmpl,url,false))<0||(req=rc=sceHttpCreateRequestWithURL(conn,verb,url,n))<0)goto done;
 if(token&&*token){char auth[RD_TOKEN_CAP+8];snprintf(auth,sizeof(auth),"Bearer %s",token);rc=sceHttpAddRequestHeader(req,"Authorization",auth,0);memset(auth,0,sizeof(auth));if(rc<0)goto done;}
 if((rc=sceHttpAddRequestHeader(req,"Accept-Encoding","identity",0))<0||(type&&(rc=sceHttpAddRequestHeader(req,"Content-Type",type,0))<0)||(rc=sceHttpSendRequest(req,body,n))<0||(rc=sceHttpGetStatusCode(req,status))<0)goto done;
 if(*status>=300&&*status<400){char *headers=NULL;size_t len=0;if((rc=sceHttpGetAllResponseHeaders(req,&headers,&len))<0||!headers||len>32768)goto done;int found=0;
  for(size_t p=0;p<len;){size_t end=p;while(end<len&&headers[end]!='\n')end++;if(end-p>9&&!strncasecmp(headers+p,"Location:",9)){size_t a=p+9,b=end;while(a<b&&(headers[a]==' '||headers[a]=='\t'))a++;while(b>a&&(headers[b-1]=='\r'||headers[b-1]==' '))b--;if(found++||b-a>=4096)goto done;memcpy(next,headers+a,b-a);next[b-a]=0;}p=end+1;}if(!found)goto done;result=1;goto done;
 }
 for(;;){if(cancelled())goto done;rc=sceHttpReadData(req,buf,65536);if(rc<0)goto done;if(!rc)break;if((unsigned)rc>65536||(uint64_t)rc>limit-total||sink(buf,(size_t)rc,ctx)){snprintf(error,cap,"Download Real-Debrid interrompido, demasiado grande ou sem espaco.");goto done;}total+=(unsigned)rc;}result=0;
done:if(req>=0)sceHttpDeleteRequest(req);if(conn>=0)sceHttpDeleteConnection(conn);if(tmpl>=0)sceHttpDeleteTemplate(tmpl);if(http>=0)sceHttpTerm(http);if(ssl>=0)sceSslTerm(ssl);if(pool>=0)sceNetPoolDestroy(pool);
#else
 (void)method;(void)body;(void)n;(void)type;(void)limit;(void)sink;(void)ctx;(void)total;(void)path;int rc=-1;
 snprintf(error,cap,"Real-Debrid requer uma PS4 real ou o teste Windows. HTTPS indisponivel nesta build do emulador.");
#endif
 free(buf);if(result<0&&!*error)snprintf(error,cap,"A ligacao Real-Debrid falhou (0x%08X). Confirma a Internet e a data da consola.",(unsigned)rc);return result;
}
int rd_http(const char *url,const char *method,const char *token,const void *body,size_t n,const char *type,uint64_t limit,RDSink sink,void *ctx,int *status,char *error,size_t cap){
 char current[4096],next[4096];if(strlen(url)>=sizeof(current))return -1;strcpy(current,url);
 for(int hop=0;hop<6;hop++){int rc=once(current,method,token,body,n,type,limit,sink,ctx,status,next,error,cap);if(rc<=0)return rc;
  if((token&&*token)||strcmp(method,"GET")){snprintf(error,cap,"Redirecionamento da API Real-Debrid recusado.");return -1;}
  if(next[0]=='/'&&next[1]!='/'){const char *end=strchr(current+8,'/');size_t len=end-current;if(len+strlen(next)>=sizeof(current))break;memcpy(current+len,next,strlen(next)+1);}else{if(strlen(next)>=sizeof(current))break;strcpy(current,next);}
 }snprintf(error,cap,"Demasiados redirecionamentos Real-Debrid.");return -1;
}
