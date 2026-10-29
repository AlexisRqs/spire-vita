#ifndef TEXT_SUBMIT_H
#define TEXT_SUBMIT_H
#include "elf_image.h"
int text_submit_configure(ElfImage *im);
int text_submit_single_page(void *cache,void *batch);
int text_submit_rotated_page(void *cache,void *batch,void *matrix);
int text_submit_flush_batch(void *batch);
void text_submit_report(void);
#endif
