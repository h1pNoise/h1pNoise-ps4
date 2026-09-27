#ifndef HARBOR_APP_H
#define HARBOR_APP_H
#include "core.h"
#include "platform.h"
#include "updater.h"
typedef struct {
 Mutex mu,io; Torrent torrent; char root[512],dir[600],message[512],phase[40],ip[16],pin[17];
 int loaded,busy,pause,auto_install,peers,port; uint64_t done,install_done,install_total;
 unsigned char *complete; unsigned char peer_id[20];
 int direct_busy,direct_task; char direct_message[512],direct_phase[16];
 UpdateState update;
} App;
extern App app;
void set_status(const char *phase,const char *message);
void file_path(int i,char out[700]);
int data_io(uint64_t off,void *buf,size_t size,int writing);
int import_torrent(const unsigned char *p,size_t n,char *error,size_t cap);
int begin_download(int install,char *error,size_t cap);
int begin_install(char *error,size_t cap);
void *http_server(void *unused);
#endif
