#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H
#include <stddef.h>
#include "elf_image.h"
#include <stdint.h>
typedef void *(*ImageLoad)(const unsigned char *,int,int *,int *,int *,void **);
typedef void *(*ImageAlloc)(size_t);
typedef void *(*ImageBase)(void *);
typedef size_t (*ImageSize)(void *);
void image_view_stats(unsigned *views,unsigned *bytes);
int image_view_configure(ElfImage *image);
void image_view_bind(ImageLoad original,ImageAlloc alloc,ImageBase base,ImageSize size);
void *image_view_load(const unsigned char *,int,int *,int *,int *,void **);
#endif
