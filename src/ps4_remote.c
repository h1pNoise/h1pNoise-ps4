#include "app.h"
#include "remote_pkg.h"
#include <stdbool.h>
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
#include <orbis/Net.h>
#include <orbis/Sysmodule.h>
#include <orbis/Bgft.h>
#include <orbis/UserService.h>
#include <orbis/AppInstUtil.h>
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

static int fetch_header(const char *url,RemotePkg *pkg,char *error,size_t cap){
 int pool=-1,ssl=-1,http=-1,tmpl=-1,conn=-1,req=-1,rc=-1,result=-1,status=0;
 unsigned char data[PKG_HEADER_SIZE];size_t got=0;uint64_t size=0;char *headers=NULL;size_t headers_len=0;
 if((rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL))<0)goto done;
 if((rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP))<0)goto done;
 if((pool=rc=sceNetPoolCreate("h1pNoise-link",512*1024,0))<0)goto done;
 if((ssl=rc=sceSslInit(512*1024))<0)goto done;
 if((http=rc=sceHttpInit(pool,ssl,1024*1024))<0)goto done;
 if((tmpl=rc=sceHttpCreateTemplate(http,"h1pNoise/0.1.10",2,0))<0)goto done;
 /* Never disable HTTPS verification or follow a page's redirect implicitly. */
 if((rc=sceHttpSetAutoRedirect(tmpl,0))<0||(rc=sceHttpSetConnectTimeOut(tmpl,10000000))<0||
    (rc=sceHttpSetResolveTimeOut(tmpl,10000000))<0||(rc=sceHttpSetRecvTimeOut(tmpl,10000000))<0||
    (rc=sceHttpSetSendTimeOut(tmpl,10000000))<0)goto done;
 if((conn=rc=sceHttpCreateConnectionWithURL(tmpl,url,0))<0)goto done;
 if((req=rc=sceHttpCreateRequestWithURL(conn,0,url,0))<0)goto done;
 if((rc=sceHttpAddRequestHeader(req,"Range","bytes=0-8191",0))<0||
    (rc=sceHttpAddRequestHeader(req,"Accept-Encoding","identity",0))<0)goto done;
 if((rc=sceHttpSendRequest(req,NULL,0))<0||(rc=sceHttpGetStatusCode(req,&status))<0)goto done;
 if(status!=206){snprintf(error,cap,"O servidor respondeu HTTP %d. Usa um link direto, sem login ou redirecionamento, que permita retomar downloads (HTTP Range).",status);goto done;}
 if((rc=sceHttpGetAllResponseHeaders(req,&headers,&headers_len))<0)goto done;
 if(pkg_range_total(headers,headers_len,&size)){snprintf(error,cap,"O servidor nao confirmou o tamanho e a leitura parcial do PKG.");goto done;}
 time_t start=time(NULL);
 while(got<sizeof(data)){
  if(time(NULL)-start>30){snprintf(error,cap,"O servidor demorou demasiado a enviar o cabecalho PKG.");goto done;}
  rc=sceHttpReadData(req,data+got,(uint32_t)(sizeof(data)-got));if(rc<=0)goto done;got+=(size_t)rc;
 }
 if(pkg_header_read(data,got,pkg,error,cap))goto done;
 if(size!=pkg->size){snprintf(error,cap,"O tamanho do ficheiro nao corresponde ao PKG. Usa um PKG completo, sem partes.");goto done;}
 result=0;
done:
 if(result&&!*error)snprintf(error,cap,"Nao foi possivel ler o link (0x%08X). Confirma o endereco, a ligacao e o certificado HTTPS.",(unsigned)rc);
 if(req>=0)sceHttpDeleteRequest(req);if(conn>=0)sceHttpDeleteConnection(conn);if(tmpl>=0)sceHttpDeleteTemplate(tmpl);
 if(http>=0)sceHttpTerm(http);if(ssl>=0)sceSslTerm(ssl);if(pool>=0)sceNetPoolDestroy(pool);
 return result;
}
int platform_queue_pkg(const char *url,int *task,char *error,size_t cap){
 *task=-1;RemotePkg pkg;if(fetch_header(url,&pkg,error,cap)||ps4_installer_ready(error,cap))return -1;
 int exists=0,rc=sceAppInstUtilAppExists(pkg.title_id,&exists);
 if(rc){snprintf(error,cap,"Nao foi possivel verificar a aplicacao: 0x%08X.",(unsigned)rc);return -1;}
 if(!pkg.patch&&exists){snprintf(error,cap,"A aplicacao %s ja esta instalada. Nao foi substituida.",pkg.title_id);return -1;}
 if(pkg.patch&&!exists){snprintf(error,cap,"Instala primeiro a aplicacao base %s.",pkg.title_id);return -1;}
 if((rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE))<0){snprintf(error,cap,"Utilizador da PS4 indisponivel: 0x%08X.",(unsigned)rc);return -1;}
 BgftParam64 p={0};rc=sceUserServiceGetForegroundUser(&p.user);if(rc){snprintf(error,cap,"Seleciona um utilizador na PS4 (0x%08X).",(unsigned)rc);return -1;}
 p.entitlement=5;p.id=pkg.content_id;p.url=url;p.name=pkg.content_id;p.icon="";p.playgo="0";
 p.options=ORBIS_BGFT_TASK_OPT_DISABLE_CDN_QUERY_PARAM;p.type="PS4GD";p.subtype="";p.size=pkg.size;
 int id=-1;
 /* Hand the original remote URL to the system. No app-local proxy or manifest. */
 rc=pkg.patch?sceBgftServiceIntDebugDownloadRegisterPkg((OrbisBgftDownloadParam*)&p,&id):sceBgftServiceIntDownloadRegisterTask((OrbisBgftDownloadParam*)&p,&id);
 if(rc||id<0){snprintf(error,cap,"A PS4 recusou o pedido: 0x%08X. Confirma espaco e tarefas existentes em Notificacoes.",(unsigned)rc);return -1;}
 rc=sceBgftServiceDownloadStartTask(id);
 if(rc){*task=id;snprintf(error,cap,"Pedido %d registado, mas nao iniciou (0x%08X). Abre Notificacoes > Transferencias para retomar ou cancelar; nao envies o link outra vez.",id,(unsigned)rc);return -1;}
 *task=id;return 0;
}
#endif
