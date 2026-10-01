#include "app.h"
#include "remote_pkg.h"
#include "version.h"
#include <stdbool.h>
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
#include <orbis/Net.h>
#include <orbis/Sysmodule.h>
#include <orbis/Bgft.h>
#include <orbis/UserService.h>
#include <orbis/AppInstUtil.h>
#include "ps4_user.h"
#include "ps4_bgft.h"
/* Typed declarations: the bundled SDK declares several of these as void().
   HTTP/SSL ABI: OpenOrbis and shadPS4 src/core/libraries/network/http.{h,cpp}. */
extern int32_t sceSslInit(size_t size);
extern int32_t sceSslTerm(int32_t id);
extern int32_t sceHttpInit(int32_t net,int32_t ssl,size_t size);
extern int32_t sceHttpTerm(int32_t id);
extern int32_t sceHttpCreateTemplate(int32_t id,const char *agent,int32_t version,int32_t proxy);
extern int32_t sceHttpDeleteTemplate(int32_t id);
extern int32_t sceHttpSetConnectTimeOut(int32_t id,uint32_t usec);
extern int32_t sceHttpSetResolveTimeOut(int32_t id,uint32_t usec);
extern int32_t sceHttpSetRecvTimeOut(int32_t id,uint32_t usec);
extern int32_t sceHttpSetSendTimeOut(int32_t id,uint32_t usec);
extern int32_t sceHttpSetAutoRedirect(int32_t id,int32_t enabled);
extern int32_t sceHttpCreateConnectionWithURL(int32_t id,const char *url,bool keepalive);
extern int32_t sceHttpDeleteConnection(int32_t id);
extern int32_t sceHttpCreateRequestWithURL(int32_t id,int32_t method,const char *url,uint64_t size);
extern int32_t sceHttpDeleteRequest(int32_t id);
extern int32_t sceHttpAddRequestHeader(int32_t id,const char *name,const char *value,int32_t mode);
extern int32_t sceHttpSendRequest(int32_t id,const void *body,size_t size);
extern int32_t sceHttpGetStatusCode(int32_t id,int32_t *status);
extern int32_t sceHttpGetAllResponseHeaders(int32_t id,char **headers,size_t *size);
extern int32_t sceHttpReadData(int32_t id,void *data,uint32_t size);
extern int ps4_installer_ready(char *error,size_t cap);

/* packageSize is uint64_t in the PS4 ABI; SDK 0.5.3 incorrectly uses uint32_t.
   Reference: flatz/ps4_stub_lib_maker_v2/include/bgft.h. */
typedef struct {
 int32_t user,entitlement;const char *id,*url,*ex_url,*name,*icon,*sku;
 uint32_t options;const char *playgo,*release,*type,*subtype;uint64_t size;
} BgftParam64;
_Static_assert(offsetof(BgftParam64,size)==0x60,"BGFT package size offset");
_Static_assert(sizeof(BgftParam64)==0x68,"BGFT parameter ABI");
void link_stage(const char *stage,int result){
#ifndef HARBOR_REMOTE_TEST
 /* No URL or token is written. Flush each entry before the next system call.
    Keep the descriptor open across the installer's temporary root change. */
 char path[700];snprintf(path,sizeof(path),"%s/link-debug.log",app.root);
 static FILE *file=NULL;if(!file)file=fopen(path,"a");
 if(file){fprintf(file,"%s %s 0x%08X\n",APP_VERSION,stage,(unsigned)result);fflush(file);fsync(fileno(file));}
 lock(&app.mu);snprintf(app.direct_message,sizeof(app.direct_message),"A verificar: %s",stage);unlock(&app.mu);
#else
 (void)stage;(void)result;
#endif
}

static int fetch_header(const char *url,RemotePkg *pkg,char *error,size_t cap){
 int pool=-1,ssl=-1,http=-1,tmpl=-1,conn=-1,req=-1,rc=-1,result=-1,status=0;
 unsigned char data[PKG_HEADER_SIZE];size_t got=0;uint64_t size=0;char *headers=NULL;size_t headers_len=0;
 link_stage("carregar SSL",0);
 if((rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL))<0)goto done;
 link_stage("carregar HTTP",rc);
 if((rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP))<0)goto done;
 link_stage("criar rede",rc);
 if((pool=rc=sceNetPoolCreate("h1pNoise-link",512*1024,0))<0)goto done;
 link_stage("iniciar SSL",rc);
 if((ssl=rc=sceSslInit(512*1024))<0)goto done;
 link_stage("iniciar HTTP",rc);
 if((http=rc=sceHttpInit(pool,ssl,1024*1024))<0)goto done;
 link_stage("criar modelo HTTP",rc);
 if((tmpl=rc=sceHttpCreateTemplate(http,"h1pNoise/" APP_VERSION,2,0))<0)goto done;
 /* Never disable HTTPS verification or follow a page's redirect implicitly. */
 if((rc=sceHttpSetAutoRedirect(tmpl,0))<0||(rc=sceHttpSetConnectTimeOut(tmpl,10000000))<0||
    (rc=sceHttpSetResolveTimeOut(tmpl,10000000))<0||(rc=sceHttpSetRecvTimeOut(tmpl,10000000))<0||
    (rc=sceHttpSetSendTimeOut(tmpl,10000000))<0)goto done;
 link_stage("ligar ao servidor",rc);
 if((conn=rc=sceHttpCreateConnectionWithURL(tmpl,url,0))<0)goto done;
 if((req=rc=sceHttpCreateRequestWithURL(conn,0,url,0))<0)goto done;
 if((rc=sceHttpAddRequestHeader(req,"Range","bytes=0-8191",0))<0||
    (rc=sceHttpAddRequestHeader(req,"Accept-Encoding","identity",0))<0)goto done;
 link_stage("enviar pedido HTTPS",rc);
 if((rc=sceHttpSendRequest(req,NULL,0))<0||(rc=sceHttpGetStatusCode(req,&status))<0)goto done;
 link_stage("ler resposta HTTP",status);
 if(status!=206){snprintf(error,cap,"O servidor respondeu HTTP %d. Usa um link direto, sem login ou redirecionamento, que permita retomar downloads (HTTP Range).",status);goto done;}
 if((rc=sceHttpGetAllResponseHeaders(req,&headers,&headers_len))<0)goto done;
 if(!headers||headers_len>32768||pkg_range_total(headers,headers_len,&size)){snprintf(error,cap,"O servidor nao confirmou o tamanho e a leitura parcial do PKG.");goto done;}
 link_stage("ler cabecalho PKG",rc);
 time_t start=time(NULL);
 while(got<sizeof(data)){
  if(time(NULL)-start>30){snprintf(error,cap,"O servidor demorou demasiado a enviar o cabecalho PKG.");goto done;}
  rc=sceHttpReadData(req,data+got,(uint32_t)(sizeof(data)-got));if(rc<=0||(size_t)rc>sizeof(data)-got)goto done;got+=(size_t)rc;
 }
 if(pkg_header_read(data,got,pkg,error,cap))goto done;
 if(size!=pkg->size){snprintf(error,cap,"O tamanho do ficheiro nao corresponde ao PKG. Usa um PKG completo, sem partes.");goto done;}
 result=0;
done:
 link_stage("fechar verificacao HTTP",rc);
 if(result&&!*error)snprintf(error,cap,"Nao foi possivel ler o link (0x%08X). Confirma o endereco, a ligacao e o certificado HTTPS.",(unsigned)rc);
 if(req>=0)sceHttpDeleteRequest(req);if(conn>=0)sceHttpDeleteConnection(conn);if(tmpl>=0)sceHttpDeleteTemplate(tmpl);
 if(http>=0)sceHttpTerm(http);if(ssl>=0)sceSslTerm(ssl);if(pool>=0)sceNetPoolDestroy(pool);
 return result;
}
int platform_queue_pkg(const char *url,int *task,char *error,size_t cap){
 *task=-1;char system_url[PKG_URL_CAP];
 if(pkg_bgft_url(url,system_url,sizeof(system_url),error,cap))return -1;
 RemotePkg pkg;if(fetch_header(url,&pkg,error,cap))return -1;
 link_stage("preparar extensao PKG para BGFT",0);
 link_stage("iniciar instalador",0);if(ps4_installer_ready(error,cap))return -1;
 int exists=0,rc=sceAppInstUtilAppExists(pkg.title_id,&exists);
 if(rc){snprintf(error,cap,"Nao foi possivel verificar a aplicacao: 0x%08X.",(unsigned)rc);return -1;}
 if(!pkg.patch&&exists){snprintf(error,cap,"A aplicacao %s ja esta instalada. Nao foi substituida.",pkg.title_id);return -1;}
 if(pkg.patch&&!exists){snprintf(error,cap,"Instala primeiro a aplicacao base %s.",pkg.title_id);return -1;}
 BgftParam64 p={0};if(ps4_active_user(&p.user,error,cap))return -1;
 p.entitlement=5;p.id=pkg.content_id;p.url=system_url;p.name=pkg.content_id;p.icon="";p.playgo="0";
 p.options=ORBIS_BGFT_TASK_OPT_DISABLE_CDN_QUERY_PARAM;p.type="PS4GD";p.subtype="";p.size=pkg.size;
 /* Remote server remains the source; no app-local proxy or manifest. */
 return ps4_bgft_submit(&p,pkg.patch?BGFT_SUBMIT_PATCH:BGFT_SUBMIT_BASE,task,error,cap);
}
#endif
