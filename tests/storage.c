#include "../src/storage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int query_error;
static uint64_t free_value;
int free_bytes(const char *path,uint64_t *available){(void)path;*available=free_value;return query_error;}
int main(void){
 uint64_t out=123;char error[512];
 assert(storage_from_blocks(4096,200000000,100000000,&out)==0&&out==409600000000ULL);
 assert(storage_from_blocks(4096,200000000,0,&out)==0&&out==0);
 assert(storage_from_blocks(4096,200000000,-1,&out)==0&&out==0);
 assert(storage_from_blocks(0,200000000,100,&out)!=0);
 assert(storage_from_blocks(4096,0,0,&out)!=0);
 assert(storage_from_blocks(4096,100,101,&out)!=0);
 assert(storage_from_blocks(UINT64_MAX,2,1,&out)!=0);
 query_error=-1;free_value=0;
#ifdef HARBOR_SHADPS4
 assert(storage_check("/data/pkg",100,error,sizeof(error))==0);
 assert(error[0]==0);
#else
 assert(storage_check("/data/harbor",100,error,sizeof(error))!=0);
 assert(strstr(error,"Nao foi possivel medir")!=NULL);
#endif
 query_error=0;free_value=0;
 assert(storage_check("/data/harbor",100,error,sizeof(error))!=0);
 assert(strstr(error,"Espaco insuficiente")!=NULL);
 free_value=100;
 assert(storage_check("/data/harbor",100,error,sizeof(error))==0);
 assert(storage_check("/data/harbor",101,error,sizeof(error))!=0);
 free_value=500000000000ULL;
 assert(storage_check("/data/harbor",52000000000ULL,error,sizeof(error))==0);
 puts("Storage: 12 checks passed (64-bit, unknown, full, boundary, invalid filesystem data).");
}
