#ifndef HARBOR_STORAGE_H
#define HARBOR_STORAGE_H
#include <stdint.h>
#include <stddef.h>
/* Zero means a successful query, including a genuinely full disk. */
int free_bytes(const char *path,uint64_t *available);
int storage_from_blocks(uint64_t block_size,uint64_t total_blocks,int64_t available_blocks,uint64_t *available);
int storage_check(const char *path,uint64_t required,char *error,size_t cap);
#endif
