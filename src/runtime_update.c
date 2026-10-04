#include "runtime_update.h"
#include "update_config.h"
#include "vendor/monocypher-ed25519.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
void runtime_log(const char *stage,int result){
#ifndef HARBOR_RUNTIME_TEST
 mkdir("/data/pkg",0777);FILE *f=fopen("/data/pkg/update-debug.log","a");if(f){fprintf(f,"runtime %s 0x%08X\n",stage,(unsigned)result);fflush(f);fsync(fileno(f));fclose(f);}
#else
 (void)stage;(void)result;
#endif
}

void runtime_path(uint32_t build,const char *suffix,char out[700]){snprintf(out,700,RUNTIME_ROOT "/h1pNoise-%u.%s",build,suffix);}
static int failure(char *error,size_t cap,const char *message){snprintf(error,cap,"%s",message);return -1;}
int runtime_atomic_write(const char *path,const void *data,size_t size){
 char part[720];snprintf(part,sizeof(part),"%s.part",path);FILE *f=fopen(part,"wb");if(!f)return -1;
 int bad=fwrite(data,1,size,f)!=size||fflush(f)||fsync(fileno(f));if(fclose(f))bad=1;
 if(bad){remove(part);return -1;}if(rename(part,path)){remove(part);return -1;}return 0;
}
static uint32_t pointer_read(const char *name){
 char path[700],buf[16];snprintf(path,sizeof(path),RUNTIME_ROOT "/%s",name);FILE *f=fopen(path,"rb");if(!f)return 0;
 size_t n=fread(buf,1,sizeof(buf),f);int bad=ferror(f);fclose(f);if(bad||!n||n>=sizeof(buf))return 0;
 uint64_t b=0;for(size_t i=0;i<n;i++){if(buf[i]<'0'||buf[i]>'9')return 0;b=b*10+(buf[i]-'0');if(b>2147483647)return 0;}return (uint32_t)b;
}
static int pointer_write(const char *name,uint32_t build){char path[700],buf[16];snprintf(path,sizeof(path),RUNTIME_ROOT "/%s",name);int n=snprintf(buf,sizeof(buf),"%u",build);return runtime_atomic_write(path,buf,(size_t)n);}
static void pointer_remove(const char *name){char path[700];snprintf(path,sizeof(path),RUNTIME_ROOT "/%s",name);remove(path);}
int runtime_store_manifest(const unsigned char *data,size_t size,uint32_t build,char *error,size_t cap){
 UpdateManifest m;if(update_manifest_read(data,size,UPDATE_PUBLIC_KEY,&m,error,cap)||m.build!=build)return failure(error,cap,"Metadados assinados da versao invalidos.");
 mkdir("/data/harbor",0777);mkdir(RUNTIME_ROOT,0777);char path[700];runtime_path(build,"h1p",path);
 if(runtime_atomic_write(path,data,size))return failure(error,cap,"Nao foi possivel guardar a assinatura da atualizacao.");return 0;
}
int runtime_verify(uint32_t build,UpdateManifest *m,char *error,size_t cap){
 if(build<RUNTIME_BASE_BUILD||build>2147483647)return failure(error,cap,"Versao de arranque invalida.");
 char path[700];runtime_path(build,"h1p",path);FILE *f=fopen(path,"rb");if(!f)return failure(error,cap,"Assinatura da atualizacao indisponivel.");
 unsigned char raw[UPDATE_MANIFEST_MAX+1];size_t n=fread(raw,1,sizeof(raw),f);int bad=ferror(f);fclose(f);
 if(bad||update_manifest_read(raw,n,UPDATE_PUBLIC_KEY,m,error,cap)||m->build!=build)return failure(error,cap,"Assinatura ou identidade da atualizacao invalida.");
 runtime_path(build,"self",path);f=fopen(path,"rb");if(!f)return failure(error,cap,"Ficheiro da atualizacao indisponivel.");
 unsigned char buffer[32768],header[8192],digest[64];size_t head=0;uint64_t total=0;crypto_sha512_ctx hash;crypto_sha512_init(&hash);
 while((n=fread(buffer,1,sizeof(buffer),f))){if(total>m->size||n>m->size-total){fclose(f);return failure(error,cap,"Tamanho da atualizacao invalido.");}if(!head){head=n<sizeof(header)?n:sizeof(header);memcpy(header,buffer,head);}crypto_sha512_update(&hash,buffer,n);total+=n;}
 bad=ferror(f);fclose(f);crypto_sha512_final(&hash,digest);
 if(bad||total!=m->size||crypto_verify64(digest,m->sha512)||update_pkg_metadata(header,head,total,m))return failure(error,cap,"A atualizacao nao corresponde ao ficheiro assinado.");return 0;
}
int runtime_activate(uint32_t build,char *error,size_t cap){
 UpdateManifest m;if(runtime_verify(build,&m,error,cap))return -1;
 uint32_t active=pointer_read("active");if(build<=active)return failure(error,cap,"A versao ja foi ativada. Nada foi substituido.");
 /* Never change active here: the candidate must survive startup first. */
 if(pointer_write("pending",build))return failure(error,cap,"Nao foi possivel preparar o arranque da nova versao.");
 pointer_remove("attempt");runtime_log("nova versao preparada",(int)build);return 0;
}
int runtime_select(char out[700]){
 strcpy(out,"/app0/h1pNoise.self");char error[512];UpdateManifest m;
 uint32_t pending=pointer_read("pending"),attempt=pointer_read("attempt"),active=pointer_read("active");
 if(pending>active&&pending>RUNTIME_BASE_BUILD&&attempt!=pending&&!runtime_verify(pending,&m,error,sizeof(error))){
  /* A crash or interrupted launch leaves attempt, so the next boot falls back. */
  if(!pointer_write("attempt",pending)){runtime_path(pending,"self",out);return 1;}
 }
 if(active>RUNTIME_BASE_BUILD&&!runtime_verify(active,&m,error,sizeof(error))){runtime_path(active,"self",out);return 2;}
 return 0;
}
int runtime_boot_confirm(uint32_t build){
 if(pointer_read("pending")!=build||pointer_read("attempt")!=build)return 0;
 char error[512];UpdateManifest m;if(runtime_verify(build,&m,error,sizeof(error)))return -1;
 if(pointer_write("active",build))return -1;pointer_remove("pending");pointer_remove("attempt");runtime_log("arranque confirmado",(int)build);return 0;
}
