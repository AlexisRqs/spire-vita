#ifndef PREVIEW_AUDIO_H
#define PREVIEW_AUDIO_H
#include "elf_image.h"
char *preview_audio_getenv(const char *name);
int preview_audio_configure(ElfImage *image);
#endif
