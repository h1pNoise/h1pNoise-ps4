/* Test accessors only. Production sources are linked unchanged (PS4, 64-bit ABI). */
#include "../src/app.h"
App app;
void link_stage(const char *stage,int result){(void)stage;(void)result;}
void fixture_reset(void){memset(&app,0,sizeof(app));strcpy(app.root,"/data/pkg");app.update.task=-1;}
void fixture_busy(int busy){app.busy=busy;}
int fixture_state(int field){switch(field){case 0:return app.update.busy;case 1:return app.update.available;case 2:return app.update.ready;case 3:return app.update.task;default:return -9;}}
const char *fixture_phase(void){return app.update.phase;}
const char *fixture_message(void){return app.update.message;}
uint64_t fixture_done(void){return app.update.done;}
const char *fixture_update_path(void){return app.update.path;}
void fixture_directory(void *p,int directory){struct stat *s=p;memset(s,0,sizeof(*s));s->st_mode=directory?S_IFDIR:S_IFREG;}
