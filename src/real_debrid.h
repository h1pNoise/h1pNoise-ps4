#ifndef H1PNOISE_REAL_DEBRID_H
#define H1PNOISE_REAL_DEBRID_H
#include "platform.h"
#define RD_TOKEN_CAP 257
typedef int (*RDSink)(const void *,size_t,void *);
/* API calls never redirect. Download redirects never carry Authorization. */
int rd_http(const char *url,const char *method,const char *token,const void *body,size_t n,
            const char *type,uint64_t limit,RDSink sink,void *ctx,int *status,char *error,size_t cap);
int rd_configure(const char *token,size_t n,char *error,size_t cap);
void rd_load(void);
int rd_enable(int enabled,char *error,size_t cap);
int rd_forget(char *error,size_t cap);
int rd_magnet_parse(const char *url,size_t n,char *error,size_t cap);
void rd_trace(const char *event,int first,int second);
void *rd_prepare_magnet(void *unused);
void *rd_download_worker(void *unused);
int torrent_verify_download(char *error,size_t cap);
int torrent_install_all(void);
#endif
