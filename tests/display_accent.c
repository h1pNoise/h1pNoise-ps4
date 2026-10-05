/* Exercise the production software renderer and its contrast calculation. */
#include "../src/display.c"
#undef NDEBUG
#include <assert.h>
static uint32_t first[W*H],second[W*H];
int main(int argc,char **argv){
 DisplayState s={0};s.accent=0x8cc8ff;s.loaded=1;s.busy=1;s.done=25;s.total=100;s.peers=3;s.space_known=1;s.available=302000000000ull;s.port=8787;
 strcpy(s.name,"Exemplo homebrew.pkg");strcpy(s.phase,"downloading");strcpy(s.message,"A descarregar para o disco interno.");strcpy(s.ip,"192.168.1.221");strcpy(s.pin,"1234");
 uint8_t qr[qrcodegen_BUFFER_LEN_MAX],temp[qrcodegen_BUFFER_LEN_MAX];
 assert(qrcodegen_encodeText("http://192.168.1.221:8787/#code=1234",temp,qr,qrcodegen_Ecc_LOW,1,10,qrcodegen_Mask_AUTO,1));
 display_render(first,&s,qr,1);assert(first[377*W+100]==0xff8cc8ff);
 s.accent=0xc5b0ff;display_render(second,&s,qr,1);assert(second[377*W+100]==0xffc5b0ff);
 int changes=0;for(int i=0;i<W*H;i++)changes+=first[i]!=second[i];assert(changes>1000);
 for(int y=34;y<122;y++)for(int x=52;x<140;x++)assert(first[y*W+x]==second[y*W+x]);
 for(int y=264;y<540;y++)for(int x=866;x<1198;x++)assert(first[y*W+x]==second[y*W+x]);
 for(int y=678;y<711;y++)for(int x=754;x<1224;x++)assert(second[y*W+x]==BG);
 for(unsigned n=0;n<256;n++){
  uint32_t color=0xff000000|n*0x010101,soft=tint(color,PANEL),background=luminance(soft)>luminance(PANEL)?soft:PANEL;
  uint32_t label=readable(color,background);assert((luminance(label)+50000ull)*2>=(luminance(background)+50000ull)*9);
 }
 if(argc>1){FILE *f=fopen(argv[1],"wb");assert(f);assert(fwrite(second,sizeof(uint32_t),W*H,f)==W*H);assert(!fclose(f));}
 puts("TV renderer checks passed: live accent, progress, dark-colour contrast, unchanged QR/logo and removed rest-mode footer.");return 0;
}
