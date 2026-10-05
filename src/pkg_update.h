#ifndef H1PNOISE_PKG_UPDATE_H
#define H1PNOISE_PKG_UPDATE_H
#include "update_manifest.h"
#include <stdio.h>
int update_file_verify(const char *,const UpdateManifest *,char *,size_t);
int update_file_verify_open(FILE *,const UpdateManifest *,char *,size_t);
#endif
