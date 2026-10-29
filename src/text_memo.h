#ifndef TEXT_MEMO_H
#define TEXT_MEMO_H
#include "elf_image.h"
#include "compat_libc.h"
int text_memo_configure(ElfImage *im);
void *text_memo_get(void *font,void *data,void *text,void *color);
void text_memo_report(void);
/* Returns -1 once any memo lookup has started. */
int text_cache_capacity_enable(int enabled);
void text_vertices_enable(int enabled);
void ANDROID_ABI text_vertices_add(void *cache,void *layout,float x,float y,void (ANDROID_ABI *original)(void*,void*,float,float));
#endif
