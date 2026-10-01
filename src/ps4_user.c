#include "ps4_user.h"
#include <stdio.h>
#include <orbis/UserService.h>
#include <orbis/Sysmodule.h>
extern void link_stage(const char *stage,int result);

int ps4_active_user(int32_t *user,char *error,size_t cap){
 *user=ORBIS_USER_SERVICE_USER_ID_INVALID;
 int rc=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE);
 link_stage("carregar servico de utilizadores",rc);
 if(rc<0){snprintf(error,cap,"Servico de utilizadores indisponivel: 0x%08X.",(unsigned)rc);return -1;}
 OrbisUserServiceInitializeParams params={.priority=ORBIS_KERNEL_PRIO_FIFO_NORMAL};
 rc=sceUserServiceInitialize(&params);
 link_stage("inicializar servico de utilizadores",rc);
 /* Loading the module alone does not initialize its service. Another app
    subsystem may already have initialized it; this is also a valid state. */
 if(rc && (uint32_t)rc!=ORBIS_USER_SERVICE_ERROR_ALREADY_INITIALIZED){
  snprintf(error,cap,"Nao foi possivel iniciar o servico de utilizadores: 0x%08X.",(unsigned)rc);return -1;
 }
 rc=sceUserServiceGetForegroundUser(user);
 link_stage("consultar utilizador ativo",rc);
 if(rc){
  *user=ORBIS_USER_SERVICE_USER_ID_INVALID;
  if((uint32_t)rc==ORBIS_USER_SERVICE_ERROR_NOT_LOGGED_IN)
   snprintf(error,cap,"Seleciona um utilizador na PS4 e volta a abrir a app (0x%08X).",(unsigned)rc);
  else snprintf(error,cap,"Nao foi possivel consultar o utilizador ativo: 0x%08X.",(unsigned)rc);
  return -1;
 }
 if(*user==ORBIS_USER_SERVICE_USER_ID_INVALID || *user==ORBIS_USER_SERVICE_USER_ID_SYSTEM){
  *user=ORBIS_USER_SERVICE_USER_ID_INVALID;
  link_stage("utilizador ativo invalido",(int32_t)ORBIS_USER_SERVICE_ERROR_NOT_LOGGED_IN);
  snprintf(error,cap,"Seleciona um utilizador na PS4 e volta a abrir a app.");return -1;
 }
 return 0;
}
