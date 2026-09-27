#include "app.h"
#include "remote_pkg.h"
int remote_pkg_supported(void){
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 return 1;
#else
 return 0;
#endif
}
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
static void *remote_worker(void *arg){
 char error[512]={0};int task=-1;int rc=platform_queue_pkg(arg,&task,error,sizeof(error));free(arg);
 lock(&app.mu);app.direct_busy=0;app.direct_task=task;snprintf(app.direct_phase,sizeof(app.direct_phase),"%s",rc?"error":"queued");
 if(rc)snprintf(app.direct_message,sizeof(app.direct_message),"%s",error);
 else snprintf(app.direct_message,sizeof(app.direct_message),"Pedido %d enviado para as Transferencias da PS4. Confirma que o download iniciou em Notificacoes > Transferencias antes de fechar a app ou testar o repouso.",task);
 unlock(&app.mu);return NULL;
}
#endif
int begin_remote_pkg(const char *url,size_t len,char *error,size_t cap){
 if(pkg_url_valid(url,len,error,cap))return -1;
 if(!remote_pkg_supported()){snprintf(error,cap,"O envio para as Transferencias requer uma PS4 real. Windows e shadPS4 nao fazem esta instalacao.");return -1;}
#if defined(__ORBIS__) && !defined(HARBOR_SHADPS4)
 char *copy=malloc(len+1);if(!copy){snprintf(error,cap,"Sem memoria para verificar o link.");return -1;}memcpy(copy,url,len);copy[len]=0;
 lock(&app.mu);if(app.busy||app.direct_busy||app.update.busy||app.update.task>=0){unlock(&app.mu);free(copy);snprintf(error,cap,"Aguarda ou pausa a operacao atual antes de enviar um link.");return -1;}
 app.direct_busy=1;app.direct_task=-1;strcpy(app.direct_phase,"checking");snprintf(app.direct_message,sizeof(app.direct_message),"A verificar o PKG do link. Mantem a aplicacao aberta.");unlock(&app.mu);
 Thread t;if(thread_start(&t,remote_worker,copy)){free(copy);lock(&app.mu);app.direct_busy=0;strcpy(app.direct_phase,"error");snprintf(app.direct_message,sizeof(app.direct_message),"Nao foi possivel verificar o link.");unlock(&app.mu);snprintf(error,cap,"Nao foi possivel iniciar a verificacao.");return -1;}pthread_detach(t);return 0;
#else
 return -1;
#endif
}
