#ifndef HARBOR_CORE_H
#define HARBOR_CORE_H
#include <stdint.h>
#include <stddef.h>
#define MAX_FILES 32
#define MAX_TRACKERS 24
#define MAX_SOURCES 16
#define MAX_TORRENT (8*1024*1024)
#define MAX_PIECE (16*1024*1024)
typedef struct { const unsigned char *data; size_t start,end,body,len; int kind,next; int64_t number; } BNode;
typedef struct { const unsigned char *data; size_t size,pos; BNode *nodes; int count,cap; } BDoc;
typedef struct { char name[512]; uint64_t size,offset; } TFile;
typedef struct {
 char name[256],hashhex[41]; unsigned char hash[20], *hashes;
 uint64_t total; uint32_t piece_size,pieces; int nfiles,ntrackers,nsources;
 TFile files[MAX_FILES]; char trackers[MAX_TRACKERS][1024];
 struct {char host[256];int port;} sources[MAX_SOURCES];
} Torrent;
int bparse(BDoc *d,const unsigned char *p,size_t n);
int bparse_prefix(BDoc *d,const unsigned char *p,size_t n,size_t *used);
void bfree(BDoc *d);
int bget(BDoc *d,int parent,const char *key);
int bstr(BDoc *d,int id,char *out,size_t cap);
int torrent_parse(Torrent *t,const unsigned char *p,size_t n,char *error,size_t cap);
void torrent_free(Torrent *t);
void sha1(const void *p,size_t n,unsigned char out[20]);
uint32_t be32(const void *p);
void put32(void *p,uint32_t v);
void put64(void *p,uint64_t v);
size_t jsonstr(char *out,size_t cap,const char *s);
#endif
