#include "installer_access.h"
#include "vendor/libjbc/jailbreak.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
extern void link_stage(const char *stage,int result);
static pthread_mutex_t credential_mutex=PTHREAD_MUTEX_INITIALIZER;
int installer_credentials_trylock(void){return pthread_mutex_trylock(&credential_mutex);}
void installer_credentials_unlock(void){pthread_mutex_unlock(&credential_mutex);}

static int with_access(int (*initialize)(char *,size_t),char *error,size_t cap){
 /* FTP and apps can have different filesystem roots. Restore credentials
    before returning, including when initialization or activation fails. */
 int fd=open("/system/common/lib/libSceAppInstUtil.sprx",O_RDONLY);
 if(fd>=0){close(fd);return initialize(error,cap);}
 struct jbc_cred saved,elevated;
 link_stage("verificar acesso libjbc",0);
 if(jbc_get_cred(&saved)){
  link_stage("libjbc indisponivel",-1);
  snprintf(error,cap,"O HEN nao disponibilizou o acesso necessario ao instalador. Envia o link-debug.log.");return -1;
 }
 elevated=saved;
 if(jbc_jailbreak_cred(&elevated)){
  int reason=jbc_resolve_error();
  link_stage("libjbc credenciais nao resolvidas",reason);
  snprintf(error,cap,"Nao foi possivel resolver o acesso aos modulos do sistema (etapa %d). Envia o link-debug.log.",reason);return -1;
 }
 link_stage("ativar acesso temporario ao instalador",0);
 if(jbc_set_cred(&elevated)){
  int restored=jbc_set_cred(&saved);link_stage("restaurar acesso apos falha",restored);
  snprintf(error,cap,restored?"Nao foi possivel restaurar o acesso normal. Fecha e volta a abrir a app.":"Nao foi possivel ativar o acesso ao instalador. Envia o link-debug.log.");return -1;
 }
 link_stage("acesso temporario ativado",0);
 int rc=initialize(error,cap);
 int restored=jbc_set_cred(&saved);link_stage("restaurar acesso normal",restored);
 if(restored){snprintf(error,cap,"Nao foi possivel restaurar o acesso normal. Fecha e volta a abrir a app.");return -1;}
 return rc;
}

static int with_permissions(int (*operation)(void *,char *,size_t),void *context,char *error,size_t cap){
 struct jbc_cred saved,authorized;
 if(jbc_get_cred(&saved)){
  link_stage("ler permissoes BGFT",-1);
  snprintf(error,cap,"Nao foi possivel obter as permissoes do instalador. Envia o link-debug.log.");return -1;
 }
 authorized=saved;
 /* flatz's documented installer auth: ShellCore ID and system capability.
    Keep the filesystem roots and all remaining capabilities unchanged. */
 authorized.sceProcType=UINT64_C(0x3800000000000010);
 authorized.sonyCred|=UINT64_C(1)<<62;
 int rc=jbc_set_auth(&authorized);link_stage("ativar permissoes BGFT",rc);
 if(rc){
  int restored=jbc_set_auth(&saved);link_stage("restaurar permissoes apos falha",restored);
  snprintf(error,cap,restored?"Falha ao restaurar permissoes. Fecha a app e consulta as Transferencias antes de repetir.":"Nao foi possivel ativar as permissoes BGFT. Envia o link-debug.log.");return -1;
 }
 rc=operation(context,error,cap);
 int restored=jbc_set_auth(&saved);link_stage("restaurar permissoes BGFT",restored);
 if(restored){snprintf(error,cap,"Falha ao restaurar permissoes. Fecha a app e consulta as Transferencias antes de repetir.");return -1;}
 return rc;
}
int installer_with_access(int (*initialize)(char *,size_t),char *error,size_t cap){
 if(pthread_mutex_lock(&credential_mutex)){snprintf(error,cap,"Nao foi possivel reservar o acesso ao instalador.");return -1;}
 int rc=with_access(initialize,error,cap);installer_credentials_unlock();return rc;
}
int installer_with_permissions(int (*operation)(void *,char *,size_t),void *context,char *error,size_t cap){
 if(pthread_mutex_lock(&credential_mutex)){snprintf(error,cap,"Nao foi possivel reservar as permissoes do instalador.");return -1;}
 int rc=with_permissions(operation,context,error,cap);installer_credentials_unlock();return rc;
}
