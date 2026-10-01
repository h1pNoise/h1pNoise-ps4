#include "ps4_bgft.h"
#include "installer_access.h"
#include <stdio.h>
#include <stdint.h>
#include <orbis/Bgft.h>
extern void link_stage(const char *stage,int result);
typedef struct {void *params;BgftSubmitKind kind;int *task;} Submission;

static int register_and_start(void *context,char *error,size_t cap){
 Submission *s=context;int id=-1,rc;
 link_stage("registar transferencia",0);
 if(s->kind==BGFT_SUBMIT_STORAGE)rc=sceBgftServiceIntDownloadRegisterTaskByStorageEx(s->params,&id);
 else if(s->kind==BGFT_SUBMIT_PATCH)rc=sceBgftServiceIntDebugDownloadRegisterPkg(s->params,&id);
 else rc=sceBgftServiceIntDownloadRegisterTask(s->params,&id);
 link_stage("resultado registo BGFT",rc);
 if(rc){
  if((uint32_t)rc==0x80990007u)snprintf(error,cap,"A PS4 recusou as permissoes do instalador BGFT (0x%08X). Envia o link-debug.log.",(unsigned)rc);
  else if((uint32_t)rc==0x80990015u)snprintf(error,cap,"Ja existe um pedido para este conteudo (0x%08X). Consulta as Transferencias antes de repetir.",(unsigned)rc);
  else if((uint32_t)rc==0x80990039u)snprintf(error,cap,"O sistema indicou falta de espaco para esta instalacao (0x%08X). Consulta o armazenamento da PS4.",(unsigned)rc);
  else if((uint32_t)rc==0x80990088u)snprintf(error,cap,"A aplicacao ja esta instalada (0x80990088). Para atualizar a h1pNoise, fecha a app e instala o seu PKG manualmente por cima da versao anterior.");
  else snprintf(error,cap,"A PS4 recusou o registo BGFT: 0x%08X. Envia o link-debug.log.",(unsigned)rc);
  return -1;
 }
 if(id<0){snprintf(error,cap,"O BGFT nao devolveu um numero de pedido valido. Envia o link-debug.log.");return -1;}
 /* Preserve a registered ID on start/restoration failure. Never retry the
    registration or cancel an existing task automatically. */
 *s->task=id;
 link_stage("iniciar transferencia",0);
 rc=sceBgftServiceDownloadStartTask(id);link_stage("resultado arranque BGFT",rc);
 if(rc){snprintf(error,cap,"Pedido %d registado, mas nao iniciou (0x%08X). Abre Notificacoes > Transferencias para retomar ou cancelar; nao envies o link outra vez.",id,(unsigned)rc);return -1;}
 return 0;
}
int ps4_bgft_submit(void *params,BgftSubmitKind kind,int *task,char *error,size_t cap){
 *task=-1;Submission s={.params=params,.kind=kind,.task=task};
 return installer_with_permissions(register_and_start,&s,error,cap);
}
