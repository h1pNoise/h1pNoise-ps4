#include "../src/installer_access.h"
extern int installer_init_mock(char *,size_t);
extern int installer_op_mock(void *,char *,size_t);
int run_installer_access(char *error,size_t cap){
 return installer_with_access(installer_init_mock,error,cap);
}
int run_installer_permissions(char *error,size_t cap){
 return installer_with_permissions(installer_op_mock,(void *)42,error,cap);
}
