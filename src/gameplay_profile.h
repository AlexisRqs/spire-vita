#ifndef GAMEPLAY_PROFILE_H
#define GAMEPLAY_PROFILE_H
#include "elf_image.h"
int gameplay_sprite_profile_enable(int active);
int gameplay_profile_configure(ElfImage *im);
void gameplay_profile_report(void);
void gameplay_experiment_enable(int enable);
void gameplay_experiment_present(unsigned frame);
#endif
