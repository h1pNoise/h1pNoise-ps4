#include "platform.h"
#ifdef _WIN32
#include <bcrypt.h>
void mutex_init(Mutex *m){InitializeCriticalSection(m);}void lock(Mutex *m){EnterCriticalSection(m);}void unlock(Mutex *m){LeaveCriticalSection(m);}
typedef struct{void *(*fn)(void*);void *arg;} Start;
static DWORD WINAPI thunk(LPVOID p){Start s=*(Start*)p;free(p);s.fn(s.arg);return 0;}
int thread_start(Thread *t,void *(*fn)(void*),void *arg){Start *s=malloc(sizeof(*s));if(!s)return -1;s->fn=fn;s->arg=arg;*t=CreateThread(NULL,0,thunk,s,0,NULL);if(!*t){free(s);return -1;}return 0;}
void thread_join(Thread t){WaitForSingleObject(t,INFINITE);CloseHandle(t);}void sleep_ms(int ms){Sleep(ms);}
int platform_init(char ip[16]){WSADATA w;if(WSAStartup(MAKEWORD(2,2),&w))return -1;char host[256];if(gethostname(host,sizeof(host))||!host[0]){strcpy(ip,"127.0.0.1");return 0;}struct addrinfo hints={0},*r=NULL;hints.ai_family=AF_INET;if(getaddrinfo(host,NULL,&hints,&r)||!r){strcpy(ip,"127.0.0.1");return 0;}struct sockaddr_in *a=(struct sockaddr_in*)r->ai_addr;inet_ntop(AF_INET,&a->sin_addr,ip,16);freeaddrinfo(r);return 0;}
int random_bytes(void *p,size_t n){return BCryptGenRandom(NULL,p,(ULONG)n,BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0?-1:0;}
int free_bytes(const char *p,uint64_t *available){ULARGE_INTEGER v;*available=0;if(!GetDiskFreeSpaceExA(p,&v,NULL,NULL))return -1;*available=v.QuadPart;return 0;}
void sock_timeout(Sock s,int sec){DWORD v=sec*1000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&v,sizeof(v));setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&v,sizeof(v));}
int install_pkg(const char *p,const char *name,char *e,size_t cap,void(*cb)(uint64_t,uint64_t)){(void)p;(void)name;(void)cb;snprintf(e,cap,"Instalacao disponivel apenas na PS4. O teste Windows nao instala PKG.");return -1;}
#else
void mutex_init(Mutex *m){pthread_mutex_init(m,NULL);}void lock(Mutex *m){pthread_mutex_lock(m);}void unlock(Mutex *m){pthread_mutex_unlock(m);}
int thread_start(Thread *t,void *(*fn)(void*),void *arg){return pthread_create(t,NULL,fn,arg);}void thread_join(Thread t){pthread_join(t,NULL);}void sleep_ms(int ms){usleep(ms*1000);}
#ifdef __ORBIS__
#include <sys/statfs.h>
int free_bytes(const char *p,uint64_t *available){
 /* OpenOrbis 0.5.3 statvfs() does not copy the filesystem fields out;
    statfs() also loses the fstatfs return value. Use fstatfs directly.
    Extra room accommodates the native FreeBSD structure's reserved tail. */
 union {struct statfs info;unsigned char room[4096];} v={0};
 *available=0;int fd=open(p,O_RDONLY);if(fd<0)return -1;
 int rc=fstatfs(fd,&v.info);close(fd);if(rc)return -1;
 return storage_from_blocks(v.info.f_bsize,v.info.f_blocks,v.info.f_bavail,available);
}
#else
int free_bytes(const char *p,uint64_t *available){
 struct statvfs v={0};*available=0;if(statvfs(p,&v)||!v.f_frsize||v.f_blocks>UINT64_MAX/v.f_frsize||v.f_bavail>v.f_blocks)return -1;
 *available=(uint64_t)v.f_bavail*v.f_frsize;return 0;
}
#endif
void sock_timeout(Sock s,int sec){
#ifdef HARBOR_SHADPS4
 /* shadPS4 v0.18 routes POSIX socket options through the SceNet ABI.
    BSD 0x1006 aborts in NameOf(); SceNet uses 0x1106 and int microseconds. */
 int usec=sec*1000000;setsockopt(s,SOL_SOCKET,0x1106,&usec,sizeof(usec));setsockopt(s,SOL_SOCKET,0x1105,&usec,sizeof(usec));
#else
 struct timeval v={sec,0};setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&v,sizeof(v));setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,&v,sizeof(v));
#endif
}
#endif
int resolve4(const char *host,struct in_addr *out){
 if(inet_pton(AF_INET,host,out)==1)return 0;
#ifdef __ORBIS__
 extern int ps4_resolve(const char*,struct in_addr*);return ps4_resolve(host,out);
#else
 struct addrinfo hints={0},*r=NULL;hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;
 if(getaddrinfo(host,NULL,&hints,&r)||!r)return -1;*out=((struct sockaddr_in*)r->ai_addr)->sin_addr;freeaddrinfo(r);return 0;
#endif
}
Sock tcp_connect(const char *host,int port,int timeout){
 struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons(port);if(resolve4(host,&a.sin_addr))return BADSOCK;
 Sock s=socket(AF_INET,SOCK_STREAM,0);if(s==BADSOCK)return s;sock_timeout(s,timeout);
#ifdef _WIN32
 u_long nonblock=1;ioctlsocket(s,FIONBIO,&nonblock);
#else
 int flags=fcntl(s,F_GETFL,0);fcntl(s,F_SETFL,flags|O_NONBLOCK);
#endif
 int ret=connect(s,(struct sockaddr*)&a,sizeof(a));
 if(ret){fd_set wr;FD_ZERO(&wr);FD_SET(s,&wr);struct timeval tv={timeout,0};ret=select((int)s+1,NULL,&wr,NULL,&tv);int err=1;
#ifdef _WIN32
 int len=sizeof(err);
#else
 socklen_t len=sizeof(err);
#endif
 if(ret<=0||getsockopt(s,SOL_SOCKET,SO_ERROR,(void*)&err,&len)||err){sockclose(s);return BADSOCK;}}
#ifdef _WIN32
 nonblock=0;ioctlsocket(s,FIONBIO,&nonblock);
#else
 fcntl(s,F_SETFL,flags);
#endif
 return s;
}
int send_all(Sock s,const void *v,size_t n){const char *p=v;while(n){int k=send(s,p,(int)n,0);if(k<=0)return -1;p+=k;n-=k;}return 0;}
int recv_all(Sock s,void *v,size_t n){char *p=v;while(n){int k=recv(s,p,(int)n,0);if(k<=0)return -1;p+=k;n-=k;}return 0;}
