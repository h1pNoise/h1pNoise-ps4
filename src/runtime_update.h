#ifndef H1PNOISE_RUNTIME_UPDATE_H
#define H1PNOISE_RUNTIME_UPDATE_H
#include "update_manifest.h"
#define RUNTIME_ROOT "/data/harbor/runtime"
#define RUNTIME_BASE_BUILD 39
void runtime_path(uint32_t build,const char *suffix,char out[700]);
int runtime_store_manifest(const unsigned char *data,size_t size,uint32_t build,char *error,size_t cap);
int runtime_verify(uint32_t build,UpdateManifest *manifest,char *error,size_t cap);
int runtime_activate(uint32_t build,char *error,size_t cap);
int runtime_select(char out[700]);
int runtime_boot_confirm(uint32_t build);
int runtime_atomic_write(const char *path,const void *data,size_t size);
void runtime_log(const char *stage,int result);
#endif
