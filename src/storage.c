#include "storage.h"
#include <stdio.h>
int storage_from_blocks(uint64_t block_size,uint64_t total_blocks,int64_t available_blocks,uint64_t *available){
 *available=0;
 if(!block_size||!total_blocks||total_blocks>UINT64_MAX/block_size)return -1;
 if(available_blocks<=0)return 0;
 if((uint64_t)available_blocks>total_blocks)return -1;
 *available=(uint64_t)available_blocks*block_size;return 0;
}
int storage_check(const char *path,uint64_t required,char *error,size_t cap){
 uint64_t available;
 if(free_bytes(path,&available)){
#if defined(__ORBIS__) || defined(HARBOR_SHADPS4)
  /* A failed query is unknown capacity, not a full disk.
     Actual write/flush failures still stop the download in data_io(). */
  if(cap)error[0]=0;
  return 0;
#else
  snprintf(error,cap,"Nao foi possivel medir o espaco livre em %s. Isto nao significa que o disco esteja cheio.",path);return -1;
#endif
 }
 if(available<required){
  snprintf(error,cap,"Espaco insuficiente em %s: %.2f GB livres; %.2f GB necessarios (inclui margem de 64 MiB).",path,available/1e9,required/1e9);return -1;
 }
 return 0;
}
