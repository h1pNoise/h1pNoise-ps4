#pragma once
#include <stddef.h>
typedef enum {BGFT_SUBMIT_BASE,BGFT_SUBMIT_PATCH,BGFT_SUBMIT_STORAGE} BgftSubmitKind;
int ps4_bgft_submit(void *params,BgftSubmitKind kind,int *task,char *error,size_t cap);
