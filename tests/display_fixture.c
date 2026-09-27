#include "../src/display.h"
#include "../src/pairing.h"
#include <string.h>
static uint32_t pixels[1280*720];
uint32_t *render_preview(int scene){
 DisplayState s={0};s.port=8899;s.available=186400000000ULL;s.space_known=1;s.direct_task=-1;
 strcpy(s.ip,"127.0.0.1");strcpy(s.pin,"0123456789abcdef");strcpy(s.phase,"idle");
 if(scene>=1&&scene<=4){
  s.loaded=1;s.busy=scene==1;s.done=1050000000;s.total=2590000000ULL;s.peers=12;
  strcpy(s.name,"Demo Homebrew Collection");strcpy(s.phase,scene==1?"downloading":scene==2?"paused":scene==3?"error":"downloaded");
  strcpy(s.message,"Demonstração visual. Os dados desta transferência são simulados.");
  if(scene==3){s.space_known=0;strcpy(s.message,"Não foi possível contactar as fontes. Tenta novamente no telemóvel.");}
  if(scene==4)s.done=s.total;
 }
 if(scene==5||scene==6){strcpy(s.direct_phase,scene==5?"queued":"error");s.direct_task=42;
  strcpy(s.message,scene==5?"Pedido enviado para as Transferências da PS4. Acompanha o progresso em Notificações. Dados de exemplo.":"Pedido registado, mas não iniciou. Confirma o estado em Notificações antes de tentar novamente.");}
 if(scene==7)strcpy(s.ip,"0.0.0.0");
 if(scene==8){s.update_available=1;strcpy(s.update_version,"0.1.11");}
 if(scene==9){s.update_available=s.update_active=1;s.update_done=3309568;s.update_size=6619136;strcpy(s.update_version,"0.1.11");strcpy(s.update_message,"A descarregar a atualização. Dados de exemplo, sem download real.");}
 uint8_t qr[PAIRING_QR_BYTES];int ok=pairing_qr(s.ip,s.port,s.pin,qr);
 display_render(pixels,&s,qr,ok);return pixels;
}
