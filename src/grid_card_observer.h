#ifndef GRID_CARD_OBSERVER_H
#define GRID_CARD_OBSERVER_H
#include "elf_image.h"
int grid_card_observer_configure(ElfImage *im);
void grid_card_observer_begin(unsigned pass,unsigned frame,int sample);
void grid_card_observer_visit(void *card);
void grid_card_observer_end(void);
int grid_card_observer_can_reuse(unsigned frame);
#endif
