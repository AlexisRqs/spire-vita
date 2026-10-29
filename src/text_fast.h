#ifndef TEXT_FAST_H
#define TEXT_FAST_H
#include "elf_image.h"
int text_fast_configure(ElfImage *im);
void text_fast_report(void);
void text_rotation_enable(int enabled);
void text_integer_enable(int enabled);
void text_profile_enable(int enabled);
#endif
