#ifndef H1PNOISE_UPDATER_H
#define H1PNOISE_UPDATER_H
#include "update_manifest.h"
typedef struct {int busy,available,ready,task,install_sent;uint32_t notified;uint64_t done;char phase[24],message[512];UpdateManifest manifest;unsigned char signed_manifest[UPDATE_MANIFEST_MAX];size_t signed_size;} UpdateState;
void updater_init(void);
int updater_begin(int operation,char *error,size_t cap); /* 0 check, 1 download, 2 install */
int updater_supported(void);
typedef int (*UpdateSink)(const unsigned char *,size_t,void *);
int update_http_get(const char *,uint64_t,UpdateSink,void *,char *,size_t);
int update_platform_install(const char *,int *,char *,size_t);
void update_notify(const char *);
int update_platform_restart(char *,size_t);
#endif
