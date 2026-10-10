#ifndef H1PNOISE_RD_JSON_H
#define H1PNOISE_RD_JSON_H
#include <stddef.h>
#include <stdint.h>
typedef struct { size_t start,end; int next,count; char type; } RDNode;
typedef struct { const char *s; size_t n,pos; RDNode *v; int count,cap; } RDJson;
int rd_json_parse(RDJson *d,const char *s,size_t n);
void rd_json_free(RDJson *d);
int rd_json_key(const RDJson *d,int node,const char *key);
int rd_json_string(const RDJson *d,int node,char *out,size_t cap);
int rd_json_uint(const RDJson *d,int node,uint64_t *out);
#endif
