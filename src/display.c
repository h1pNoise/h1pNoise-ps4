#include "display.h"
#include "ui_assets.h"
#include "version.h"
#include "vendor/qrcodegen.h"
#include <stdio.h>
#include <string.h>
#define W 1280
#define H 720
#define BG 0xff0b0e11
#define PANEL 0xff14191e
#define LINE 0xff293138
#define TEXT 0xffeef3f4
#define MUTED 0xffa1adb5
#define MINT accent_text
static uint32_t accent_text,accent_bar,accent_soft;
/* sRGB luminance scaled to one million; no runtime maths library needed. */
static const uint32_t linear_channel[256]={0,304,607,911,1214,1518,1821,2125,2428,2732,3035,3347,3677,4025,4391,4777,5182,5605,6049,6512,6995,7499,8023,8568,9134,9721,10330,10960,11612,12286,12983,13702,14444,15209,15996,16807,17642,18500,19382,20289,21219,22174,23153,24158,25187,26241,27321,28426,29557,30713,31896,33105,34340,35601,36889,38204,39546,40915,42311,43735,45186,46665,48172,49707,51269,52861,54480,56128,57805,59511,61246,63010,64803,66626,68478,70360,72272,74214,76185,78187,80220,82283,84376,86500,88656,90842,93059,95307,97587,99899,102242,104616,107023,109462,111932,114435,116971,119538,122139,124772,127438,130136,132868,135633,138432,141263,144128,147027,149960,152926,155926,158961,162029,165132,168269,171441,174647,177888,181164,184475,187821,191202,194618,198069,201556,205079,208637,212231,215861,219526,223228,226966,230740,234551,238398,242281,246201,250158,254152,258183,262251,266356,270498,274677,278894,283149,287441,291771,296138,300544,304987,309469,313989,318547,323143,327778,332452,337164,341914,346704,351533,356400,361307,366253,371238,376262,381326,386429,391572,396755,401978,407240,412543,417885,423268,428690,434154,439657,445201,450786,456411,462077,467784,473531,479320,485150,491021,496933,502886,508881,514918,520996,527115,533276,539479,545724,552011,558340,564712,571125,577580,584078,590619,597202,603827,610496,617207,623960,630757,637597,644480,651406,658375,665387,672443,679542,686685,693872,701102,708376,715694,723055,730461,737910,745404,752942,760525,768151,775822,783538,791298,799103,806952,814847,822786,830770,838799,846873,854993,863157,871367,879622,887923,896269,904661,913099,921582,930111,938686,947307,955973,964686,973445,982251,991102,1000000};
static uint32_t luminance(uint32_t c){return (linear_channel[(c>>16)&255]*2126ull+linear_channel[(c>>8)&255]*7152ull+linear_channel[c&255]*722ull)/10000;}
static uint32_t tint(uint32_t color,uint32_t base){uint32_t out=0xff000000;for(int shift=0;shift<=16;shift+=8)out|=((((color>>shift)&255)*14+((base>>shift)&255)*86)/100)<<shift;return out;}
static uint32_t readable(uint32_t color,uint32_t background){
 while((luminance(color)+50000ull)*2<(luminance(background)+50000ull)*9){uint32_t next=0xff000000;for(int shift=0;shift<=16;shift+=8){unsigned n=((color>>shift)&255)+8;if(n>255)n=255;next|=n<<shift;}color=next;}return color;
}
#define WARN 0xffeac184
static uint32_t *px;
static void rect(int x,int y,int w,int h,uint32_t c){for(int j=y;j<y+h&&j<H;j++)if(j>=0)for(int i=x;i<x+w&&i<W;i++)if(i>=0)px[j*W+i]=c;}
static void rounded(int x,int y,int w,int h,int r,uint32_t c){for(int j=0;j<h;j++){int inset=0,dy=j<r?r-j-1:j>=h-r?j-(h-r):0;if(dy){while(inset<r&&(r-inset)*(r-inset)+dy*dy>r*r)inset++;}rect(x+inset,y+j,w-2*inset,1,c);}}
static void card(int x,int y,int w,int h){rounded(x,y,w,h,16,LINE);rounded(x+1,y+1,w-2,h-2,15,PANEL);}
static void blend(int x,int y,uint32_t color,unsigned a){if(!a||x<0||x>=W||y<0||y>=H)return;uint32_t old=px[y*W+x];unsigned inv=255-a;unsigned r=(((color>>16)&255)*a+((old>>16)&255)*inv)/255,g=(((color>>8)&255)*a+((old>>8)&255)*inv)/255,b=((color&255)*a+(old&255)*inv)/255;px[y*W+x]=0xff000000|(r<<16)|(g<<8)|b;}
static unsigned character(const char **p){unsigned c=(unsigned char)*(*p)++;if(c>=0xc2&&c<=0xc3&&((unsigned char)**p&0xc0)==0x80){c=((c&31)<<6)|((unsigned char)*(*p)++&63);}else if(c>=128){while(((unsigned char)**p&0xc0)==0x80)(*p)++;c='?';}return c>=32&&c<256?c:'?';}
static int width(const char *s,int font){int w=0;while(*s)w+=ui_glyphs[font][character(&s)-32].advance;return w;}
static void text(int x,int y,const char *s,int font,uint32_t color,int limit){int edge=x+limit;while(*s){const UiGlyph *g=&ui_glyphs[font][character(&s)-32];if(x+g->advance>edge)break;for(int j=0;j<g->h;j++)for(int i=0;i<g->w;i++)blend(x+g->x+i,y+g->y+j,color,ui_coverage[g->offset+j*g->w+i]);x+=g->advance;}}
static void center(int mid,int y,const char *s,int font,uint32_t color,int max){int w=width(s,font);if(w>max)w=max;text(mid-w/2,y,s,font,color,max);}
static void wrap(int x,int y,const char *s,int font,uint32_t color,int limit,int rows){
 int line_h=font==2?36:26;
 for(int row=0;row<rows&&*s;row++){
  while(*s==' ')s++;const char *p=s,*end=s,*space=NULL;int w=0;
  while(*p){const char *before=p;unsigned c=character(&p);int a=ui_glyphs[font][c-32].advance;if(w+a>limit)break;w+=a;end=p;if(*before==' ')space=before;}
  if(*end&&space&&space>s)end=space;if(end==s)break;
  char line[512];size_t n=(size_t)(end-s);if(n>=sizeof(line))n=sizeof(line)-1;memcpy(line,s,n);line[n]=0;
  if(row==rows-1&&*end){while(n&&width(line,font)>limit-width("...",font)){n--;while(n&&((unsigned char)line[n]&0xc0)==0x80)n--;line[n]=0;}if(n+3<sizeof(line))strcat(line,"...");}
  text(x,y+row*line_h,line,font,color,limit);s=end;
 }
}
static const char *phase(const char *p){
 if(!strcmp(p,"rd-account"))return "Real-Debrid: conta";
 if(!strcmp(p,"rd-upload"))return "Real-Debrid: a enviar";
 if(!strcmp(p,"rd-waiting"))return "Real-Debrid: a preparar";
 if(!strcmp(p,"metadata"))return "A obter dados do magnet";
 if(!strcmp(p,"downloading"))return "A descarregar";if(!strcmp(p,"paused"))return "Em pausa";if(!strcmp(p,"waiting")||!strcmp(p,"trackers"))return "A procurar fontes";
 if(!strcmp(p,"checking"))return "A verificar";if(!strcmp(p,"installing"))return "A instalar";if(!strcmp(p,"downloaded"))return "Download concluído";if(!strcmp(p,"installed"))return "Instalado";if(!strcmp(p,"error"))return "Requer atenção";return "Pronto para começar";
}
void display_render(uint32_t *pixels,const DisplayState *s,const uint8_t *qr,int has_qr){
 accent_bar=0xff000000|(s->accent&0xffffff);accent_soft=tint(accent_bar,PANEL);accent_text=readable(accent_bar,luminance(accent_soft)>luminance(PANEL)?accent_soft:PANEL);
 px=pixels;rect(0,0,W,H,BG);rect(56,136,1168,1,LINE);
 for(int y=0;y<88;y++)for(int x=0;x<88;x++)px[(34+y)*W+52+x]=ui_logo[y*88+x];
 text(158,43,"h1pNoise",2,TEXT,500);
 if(s->update_available){char update[120];
  snprintf(update,sizeof(update),"Nova versão %s · Atualiza na app ou no site",s->update_version);
  text(159,84,update,0,MINT,800);}
 else text(159,84,"CENTRAL DE TRANSFERÊNCIAS",0,MUTED,600);
 rounded(1030,46,194,34,17,has_qr?accent_soft:0xff34291b);center(1127,52,has_qr?(s->emulator?"shadPS4 / teste":"PS4 ligada à rede"):"Rede indisponível",0,has_qr?MINT:WARN,178);
 text(1052,91,"h1pNoise " APP_VERSION,0,MUTED,174);
 card(56,166,758,322);card(840,166,384,482);card(56,510,366,138);card(444,510,370,138);
 int direct=!s->busy&&s->direct_phase[0];int error=direct?!strcmp(s->direct_phase,"error"):!strcmp(s->phase,"error");
 text(84,191,s->update_active?"ATUALIZAÇÃO DA APP":direct?"LINK DIRETO PKG":"TRANSFERÊNCIA ATUAL",0,MINT,430);
 const char *status=direct?(s->direct_busy?"A verificar o link":error?"Requer atenção":"Enviado para a PS4"):s->loaded?phase(s->phase):"Sem atividade";
 if(s->update_active)status="Segue na app ou no site";
 int sw=width(status,0)+28;rounded(786-sw,185,sw,32,16,error?0xff34291b:accent_soft);text(800-sw,190,status,0,error?WARN:MINT,sw-28);
 char value[120];
 if(s->update_active){
  snprintf(value,sizeof(value),"h1pNoise %s",s->update_version);text(84,244,value,3,TEXT,680);
  wrap(84,313,s->update_message,1,MUTED,702,4);
  rounded(84,436,702,7,3,LINE);if(s->update_size){double progress=(double)s->update_done/s->update_size;if(progress>1)progress=1;if(progress>0)rounded(84,436,(int)(702*progress),7,3,accent_bar);}
  text(84,456,"GoldHEN: fecha a app e desliga Background Installation.",0,MINT,702);
 }else if(direct){
  text(84,236,error?"Vamos verificar este pedido.":s->direct_busy?"A preparar a transferência.":"A PS4 trata do resto.",2,TEXT,702);
  wrap(84,292,s->message,1,error?WARN:MUTED,702,4);
  if(s->direct_task>=0){snprintf(value,sizeof(value),"Pedido %d · Notificações > Transferências",s->direct_task);text(84,447,value,0,MINT,702);}
  else if(s->direct_busy){int offset=(s->frame*11)%532;rounded(84,446,702,5,2,LINE);rounded(84+offset,446,170,5,2,accent_bar);}
 }else if(!s->loaded){
  text(84,244,"Tudo pronto.",3,TEXT,680);text(86,312,"O próximo download começa na app ou no site.",1,MUTED,700);
  rounded(84,376,702,78,12,accent_soft);text(106,390,"01  Liga-te pela app ou pelo site.",1,MINT,650);text(106,420,"02  Envia um torrent ou cola um link PKG.",1,TEXT,650);
 }else{
  wrap(84,230,s->name,2,TEXT,702,2);double ratio=s->total?(double)s->done/s->total:0;if(ratio>1)ratio=1;
  if(s->total)snprintf(value,sizeof(value),"%.1f%%",ratio*100);else snprintf(value,sizeof(value),"A preparar");text(84,308,value,s->total?3:2,TEXT,220);
  snprintf(value,sizeof(value),"%.2f / %.2f GB",s->done/1e9,s->total/1e9);if(s->total)text(286,331,value,1,MUTED,480);
  rounded(84,374,702,7,3,LINE);if(ratio>0)rounded(84,374,(int)(702*ratio),7,3,accent_bar);
  wrap(84,399,s->message,1,error?WARN:MUTED,702,2);
  snprintf(value,sizeof(value),"%d fontes ligadas  ·  Controlo na app ou no site",s->peers);text(84,457,value,0,MUTED,702);
 }
 text(82,531,"ESPAÇO LIVRE PARA TORRENTS",0,MUTED,320);
 if(s->space_known){snprintf(value,sizeof(value),"%.2f GB",s->available/1e9);text(82,558,value,2,TEXT,310);}
 else text(82,558,"Medição indisponível",2,TEXT,310);
 text(82,610,s->space_known?"A instalação precisa de espaço adicional.":"Confirma nas definições de armazenamento.",0,MUTED,315);
 text(470,531,"DESTINO DOS TORRENTS",0,MUTED,315);text(470,558,"/data/pkg",2,MINT,315);text(470,610,"Os PKG ficam guardados após instalar.",0,MUTED,315);
 center(1032,188,"Liga-te à consola",2,TEXT,334);center(1032,226,"Lê o QR ou abre o endereço",0,MUTED,326);
 if(has_qr){int count=qrcodegen_getSize(qr),scale=6,border=4,side=(count+border*2)*scale,x0=1032-side/2,y0=264;
  rounded(x0-8,y0-8,side+16,side+16,12,0xffffffff);rect(x0,y0,side,side,0xffffffff);
  for(int y=0;y<count;y++)for(int x=0;x<count;x++)if(qrcodegen_getModule(qr,x,y))rect(x0+(x+border)*scale,y0+(y+border)*scale,scale,scale,0xff000000);
  snprintf(value,sizeof(value),"http://%s:%d",s->ip,s->port);center(1032,554,value,1,MINT,332);
  snprintf(value,sizeof(value),"Código: %s",s->pin);center(1032,589,value,0,TEXT,332);
 }else{center(1032,352,"Sem ligação à rede",2,WARN,334);wrap(873,401,"Liga a PS4 à rede e volta a abrir a aplicação para gerar o QR.",1,MUTED,318,3);}
 center(1032,620,"Dispositivo e PS4 na mesma rede",0,MUTED,332);
 text(56,678,"Torrents: mantém a app aberta e a PS4 ligada.",0,MUTED,660);
}
