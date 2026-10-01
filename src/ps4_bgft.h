#pragma once
#include <stddef.h>
typedef enum {BGFT_SUBMIT_BASE,BGFT_SUBMIT_PATCH,BGFT_SUBMIT_STORAGE} BgftSubmitKind;
int ps4_bgft_submit(void *params,BgftSubmitKind kind,int *task,char *error,size_t cap);
/* Only the signed app updater may prepare replacement of its own title.
 * Called once, under installer permissions, on SAME_APPLICATION_INSTALLED. */
int ps4_bgft_submit_update(void *params,int (*prepare)(void *,char *,size_t),void *context,int *task,char *error,size_t cap);
