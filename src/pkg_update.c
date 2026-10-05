#include "pkg_update.h"
#include <stdio.h>
#include <string.h>
static int fail(char *error,size_t cap,const char *message){snprintf(error,cap,"%s",message);return -1;}
int pkg_update_request_read(const unsigned char *data,size_t size,PkgUpdateRequest *out){
 memset(out,0,sizeof(*out));if(!size||size>64)return -1;
 uint64_t pid=0;size_t i=0;while(i<size&&data[i]!='\n'){
  unsigned c=data[i++]-'0';if(c>9||pid>(2147483647u-c)/10)return -1;pid=pid*10+c;
 }
 if(pid<=1||i==size||size-i-1!=33||data[size-1]!='\n')return -1;
 i++;for(unsigned k=0;k<32;k++){unsigned c=data[i+k];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return -1;out->nonce[k]=(char)c;}
 out->source_pid=(int32_t)pid;return 0;
}
int pkg_update_run(const unsigned char *raw,size_t size,const unsigned char key[32],
 const PkgUpdateRequest *request,const PkgUpdateOps *ops,char *error,size_t cap){
 UpdateManifest m;char current[6]={0},path[700];
 if(request->source_pid<=1||strlen(request->nonce)!=32)return fail(error,cap,"Pedido de instalacao invalido.");
 ops->report("verificar assinatura",0);
 if(update_manifest_read(raw,size,key,&m,error,cap))return -1;
 if(m.build<40||strcmp(m.sfo,"00.40")<0)return fail(error,cap,"A atualizacao e anterior ao canal suportado.");
 snprintf(path,sizeof(path),"/data/pkg/h1pNoise-update-%u.pkg",m.build);
 if(update_file_verify(path,&m,error,cap))return -1;
 if(ops->installed_sfo(current,error,cap)||strlen(current)!=5)return -1;
 if(strcmp(m.sfo,current)<=0)return fail(error,cap,"A versao instalada ja e igual ou superior. Nada foi instalado.");
 int state=ops->alive(request->source_pid);
 if(state<0)return fail(error,cap,"Nao foi possivel consultar o processo da h1pNoise. A entrega nao foi confirmada.");
 ops->report("PKG verificado; aguardar fecho da app",0);
 if(ops->ack(request->nonce))return fail(error,cap,"Nao foi possivel confirmar a entrega. A app nao deve fechar.");
 int gone=0;for(unsigned i=0;i<120;i++){
  if(i)state=ops->alive(request->source_pid);if(state<0)return fail(error,cap,"Nao foi possivel confirmar que a h1pNoise fechou.");
  if(!state){gone=1;break;}ops->wait_ms(500);
 }
 if(!gone)return fail(error,cap,"A h1pNoise continua aberta. Fecha-a antes de instalar pelo GoldHEN.");
 /* Verify again after the handoff: no replacement/removal API is permitted. */
 if(update_file_verify(path,&m,error,cap)||ops->installed_sfo(current,error,cap))return -1;
 if(strcmp(m.sfo,current)<=0)return fail(error,cap,"A versao instalada mudou durante a entrega. Nada foi instalado.");
 ops->report("chamar instalacao direta, sem desinstalar",0);
 if(ops->install(path,error,cap))return -1;
 ops->report("pedido aceite; confirmar pacote instalado",0);
 for(unsigned i=0;i<120;i++){
  if(!ops->installed_matches(&m,error,cap)){ops->report("instalacao confirmada",0);return 0;}
  ops->wait_ms(500);
 }
 return fail(error,cap,"O sistema aceitou o pedido, mas a nova instalacao nao foi confirmada. O PKG foi preservado; consulta update-install-debug.log.");
}
