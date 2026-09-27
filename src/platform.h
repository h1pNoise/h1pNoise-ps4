#ifndef HARBOR_PLATFORM_H
#define HARBOR_PLATFORM_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "storage.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <direct.h>
typedef SOCKET Sock;
typedef HANDLE Thread;
typedef CRITICAL_SECTION Mutex;
#define BADSOCK INVALID_SOCKET
#define sockclose closesocket
#define fseeko _fseeki64
#define ftello _ftelli64
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
static inline int make_dir(const char *p){return _mkdir(p);}
#else
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <strings.h>
typedef int Sock;
typedef pthread_t Thread;
typedef pthread_mutex_t Mutex;
#define BADSOCK (-1)
#define sockclose close
static inline int make_dir(const char *p){return mkdir(p,0777);}
#endif
void mutex_init(Mutex *m);
void lock(Mutex *m);
void unlock(Mutex *m);
int thread_start(Thread *t,void *(*fn)(void*),void *arg);
void thread_join(Thread t);
void sleep_ms(int ms);
int platform_init(char ip[16]);
int random_bytes(void *p,size_t n);
Sock tcp_connect(const char *host,int port,int timeout);
int resolve4(const char *host,struct in_addr *out);
void sock_timeout(Sock s,int sec);
int send_all(Sock s,const void *data,size_t len);
int recv_all(Sock s,void *data,size_t len);
int install_pkg(const char *path,const char *name,char *error,size_t cap,void(*progress)(uint64_t,uint64_t));
void screen_run(const char *ip,const char *pin);
#endif
