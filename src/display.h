#ifndef H1PNOISE_DISPLAY_H
#define H1PNOISE_DISPLAY_H
#include <stdint.h>
typedef struct {
 uint32_t accent;
 int loaded,busy,installing,peers,space_known,emulator,direct_busy,direct_task,frame,port;
 uint64_t done,total,available;
 char name[256],message[512],phase[40],direct_phase[16],ip[16],pin[17];
 int update_available,update_active;uint64_t update_done,update_size;
 char update_version[32],update_message[512];
} DisplayState;
void display_render(uint32_t *pixels,const DisplayState *state,const uint8_t *qr,int has_qr);
#endif
