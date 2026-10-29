#ifndef BUFFER_POINTER_H
#define BUFFER_POINTER_H
#include "elf_image.h"
int buffer_pointer_configure(ElfImage *im);
void buffer_pointer_enable(int active);
void buffer_pointer_report(void);
#endif
