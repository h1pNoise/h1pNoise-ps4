#include "pairing.h"
#include <stdio.h>
#include <string.h>
int pairing_qr(const char *ip,int port,const char *pin,uint8_t qr[PAIRING_QR_BYTES]){
 char url[100];uint8_t temp[PAIRING_QR_BYTES];
 if(!ip||!ip[0]||!strcmp(ip,"0.0.0.0")||port<1||port>65535||!pin||strlen(pin)!=4)return 0;
 for(const char *p=ip;*p;p++)if((*p<'0'||*p>'9')&&*p!='.')return 0;
 for(const char *p=pin;*p;p++)if(*p<'0'||*p>'9')return 0;
 int n=snprintf(url,sizeof(url),"http://%s:%d/#code=%s",ip,port,pin);
 if(n<0||(size_t)n>=sizeof(url))return 0;
 return qrcodegen_encodeText(url,temp,qr,qrcodegen_Ecc_MEDIUM,1,PAIRING_QR_VERSION,qrcodegen_Mask_AUTO,true);
}
