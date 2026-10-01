#ifndef H1PNOISE_REMOTE_PKG_H
#define H1PNOISE_REMOTE_PKG_H
#include <stdint.h>
#include <stddef.h>
#define PKG_URL_CAP 2048
#define PKG_HEADER_SIZE 8192
typedef struct { char content_id[37],title_id[10]; uint64_t size; int patch; } RemotePkg;
int pkg_url_valid(const char *url,size_t len,char *error,size_t cap);
int pkg_bgft_url(const char *url,char *out,size_t out_cap,char *error,size_t cap);
int pkg_header_read(const unsigned char *data,size_t len,RemotePkg *pkg,char *error,size_t cap);
int pkg_range_total(const char *headers,size_t len,uint64_t *total);
int remote_pkg_supported(void);
int begin_remote_pkg(const char *url,size_t len,char *error,size_t cap);
int platform_queue_pkg(const char *url,int *task,char *error,size_t cap);
#endif
