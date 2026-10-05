#ifndef H1PNOISE_APPEARANCE_H
#define H1PNOISE_APPEARANCE_H
#include <stdint.h>
#include <stddef.h>
#define ACCENT_DEFAULT 0x9debcfu
int accent_parse(const char *text,size_t size,uint32_t *color);
uint32_t accent_load(const char *root);
int accent_save(const char *root,uint32_t color);
#endif
