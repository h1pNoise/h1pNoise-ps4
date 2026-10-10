#include "../src/app.h"
#include "../src/real_debrid.h"
/* Production engine, JSON, RD state machine and PS4 HTTPS client are unchanged. */
void fixture_reset(const unsigned char *hashes){
 memset(&app,0,sizeof(app));app.update.task=-1;strcpy(app.root,"/data/pkg");strcpy(app.dir,"/data/pkg/test-torrent");
 strcpy(app.torrent.name,"test.pkg");strcpy(app.torrent.hashhex,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");app.torrent.nfiles=1;strcpy(app.torrent.files[0].name,"test.pkg");app.torrent.files[0].size=256;app.torrent.total=256;app.torrent.piece_size=128;app.torrent.pieces=2;
 app.torrent.hashes=malloc(40);memcpy(app.torrent.hashes,hashes,40);app.complete=calloc(2,1);app.loaded=1;
 char error[256];rd_configure("",0,error,sizeof(error));
}
void fixture_run(int install){app.busy=1;app.rd_active=1;app.pause=0;app.auto_install=install;rd_download_worker(NULL);}
const char *fixture_phase(void){return app.phase;}
const char *fixture_message(void){return app.message;}
void fixture_pause(void){app.pause=1;}
int fixture_busy(void){return app.busy;}
uint64_t fixture_done(void){return app.done;}
int fixture_enabled(void){return app.rd_enabled;}
int fixture_configured(void){return app.rd_configured;}
int fixture_loaded(void){return app.loaded;}
void fixture_reload(void){rd_load();}
int fixture_magnet(const char *url,size_t n,char *error,size_t cap){return begin_magnet(url,n,error,cap);}
void fixture_multi(const unsigned char *hashes){
 const char *names[]={"base.pkg","update.pkg","Backport/backport.pkg"};
 free(app.torrent.hashes);free(app.complete);app.torrent.nfiles=3;app.torrent.total=3*8192;app.torrent.piece_size=128;app.torrent.pieces=3*64;
 for(int i=0;i<3;i++){strcpy(app.torrent.files[i].name,names[i]);app.torrent.files[i].size=8192;app.torrent.files[i].offset=(uint64_t)i*8192;}
 app.torrent.hashes=malloc(20*app.torrent.pieces);memcpy(app.torrent.hashes,hashes,20*app.torrent.pieces);app.complete=calloc(app.torrent.pieces,1);
}
