#ifndef H1PNOISE_UPDATE_MANIFEST_H
#define H1PNOISE_UPDATE_MANIFEST_H
#include <stdint.h>
#include <stddef.h>
#define UPDATE_MANIFEST_MAX 4096
#define UPDATE_PACKAGE_MAX (128ULL*1024*1024)
typedef struct { uint32_t build;uint64_t size;unsigned char sha512[64];char version[32],sfo[6],url[2048],notes[768]; } UpdateManifest;
/* One file: a 64-byte Ed25519 signature followed by nine LF-terminated fields. */
int update_manifest_read(const unsigned char *data,size_t size,const unsigned char public_key[32],UpdateManifest *out,char *error,size_t cap);
int update_https_url(const char *url);
int update_redirect_allowed(const char *initial,const char *next);
int update_pkg_metadata(const unsigned char *data,size_t len,uint64_t file_size,const UpdateManifest *manifest);
#endif
