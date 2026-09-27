/* Minimal declaration for testing storage policy without a host C runtime.
   Only tests built with -I tests/wasm use this header. */
#include <stddef.h>
int snprintf(char *buffer,size_t capacity,const char *format,...);
