#ifndef HARBOR_PAIRING_H
#define HARBOR_PAIRING_H
#include "vendor/qrcodegen.h"
#define PAIRING_QR_VERSION 5
#define PAIRING_QR_BYTES qrcodegen_BUFFER_LEN_FOR_VERSION(PAIRING_QR_VERSION)
/* Fragment keeps the session code out of the HTTP request URL. */
int pairing_qr(const char *ip,int port,const char *pin,uint8_t qr[PAIRING_QR_BYTES]);
#endif
