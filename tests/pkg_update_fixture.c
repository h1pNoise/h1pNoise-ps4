#include "../src/pkg_update.h"
#include <stdio.h>
extern int helper_installed_sfo(char *,char *,size_t);
extern int helper_alive(int32_t);
extern void helper_wait(unsigned);
extern int helper_ack(const char *);
extern int helper_install(const char *,char *,size_t);
extern int helper_installed_matches(const UpdateManifest *,char *,size_t);
extern void helper_report(const char *,int);
int fixture_helper(const unsigned char *raw,size_t size,const unsigned char *key,
 const unsigned char *request,size_t request_size,char *error,size_t cap){
 PkgUpdateRequest r;if(pkg_update_request_read(request,request_size,&r)){snprintf(error,cap,"Pedido invalido");return -1;}
 PkgUpdateOps ops={helper_installed_sfo,helper_alive,helper_wait,helper_ack,helper_install,helper_installed_matches,helper_report};
 return pkg_update_run(raw,size,key,&r,&ops,error,cap);
}
