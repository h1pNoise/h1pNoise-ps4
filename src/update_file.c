#include "pkg_update.h"
#include "vendor/monocypher-ed25519.h"
#include <stdio.h>
#include <stdlib.h>
int update_file_verify(const char *path,const UpdateManifest *m,char *error,size_t cap){
 FILE *f=fopen(path,"rb");if(!f){snprintf(error,cap,"A atualizacao descarregada ja nao esta disponivel.");return -1;}
 unsigned char buffer[32768],digest[64];crypto_sha512_ctx hash;crypto_sha512_init(&hash);uint64_t total=0;size_t n;
 while((n=fread(buffer,1,sizeof(buffer),f))){if(total>m->size||n>m->size-total){fclose(f);snprintf(error,cap,"O tamanho da atualizacao mudou.");return -1;}total+=n;crypto_sha512_update(&hash,buffer,n);}
 int io_error=ferror(f);crypto_sha512_final(&hash,digest);
 if(io_error||total!=m->size||crypto_verify64(digest,m->sha512)){fclose(f);snprintf(error,cap,"O ficheiro da atualizacao nao corresponde a versao assinada.");return -1;}
 unsigned char *meta=malloc(512*1024);if(!meta){fclose(f);snprintf(error,cap,"Sem memoria para verificar a atualizacao.");return -1;}
 rewind(f);n=fread(meta,1,512*1024,f);int bad=ferror(f)||update_pkg_metadata(meta,n,total,m);free(meta);fclose(f);
 if(bad){snprintf(error,cap,"O pacote nao corresponde a identidade e versao da h1pNoise.");return -1;}return 0;
}
