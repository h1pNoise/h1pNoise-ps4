#pragma once
#include <stddef.h>
int installer_with_access(int (*initialize)(char *,size_t),char *error,size_t cap);
int installer_with_permissions(int (*operation)(void *,char *,size_t),void *context,char *error,size_t cap);
/* Storage queries never wait while an installer has temporary credentials. */
int installer_credentials_trylock(void);
void installer_credentials_unlock(void);
