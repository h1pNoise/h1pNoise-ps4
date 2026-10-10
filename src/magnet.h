#ifndef H1PNOISE_MAGNET_H
#define H1PNOISE_MAGNET_H
#include "core.h"
#define MAGNET_CAP 32768
#define MAGNET_METADATA_MAX (MAX_TORRENT-32768)
typedef struct {
 Torrent torrent;
} Magnet;
int magnet_parse(Magnet *m,const char *url,size_t n,char *error,size_t cap);
int magnet_parse_service(Magnet *m,const char *url,size_t n,char *error,size_t cap);
int magnet_torrent(const Torrent *m,const unsigned char *info,size_t n,unsigned char **out,size_t *size,char *error,size_t cap);
#endif
