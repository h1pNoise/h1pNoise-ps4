#ifndef H1PNOISE_PKG_UPDATE_H
#define H1PNOISE_PKG_UPDATE_H
#include "update_manifest.h"
#define PKG_UPDATER_TITLE "HBRU00001"
#define PKG_UPDATE_ROOT "/data/harbor/pkg-updater"
typedef struct {int32_t source_pid;char nonce[33];} PkgUpdateRequest;
typedef struct {
 int (*installed_sfo)(char out[6],char *error,size_t cap);
 int (*alive)(int32_t pid); /* 0 gone, 1 alive, -1 unknown; never kills. */
 void (*wait_ms)(unsigned ms);
 int (*ack)(const char *nonce);
 int (*install)(const char *path,char *error,size_t cap);
 int (*installed_matches)(const UpdateManifest *,char *error,size_t cap);
 void (*report)(const char *stage,int result);
} PkgUpdateOps;
int update_file_verify(const char *,const UpdateManifest *,char *,size_t);
int pkg_update_request_read(const unsigned char *,size_t,PkgUpdateRequest *);
int pkg_update_run(const unsigned char *,size_t,const unsigned char key[32],
 const PkgUpdateRequest *,const PkgUpdateOps *,char *,size_t);
int pkg_update_handoff(const char *,char *,size_t);
int pkg_update_direct(const char *,char *,size_t);
#endif
