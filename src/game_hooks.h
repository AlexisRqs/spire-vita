#ifndef GAME_HOOKS_H
#define GAME_HOOKS_H
#include "elf_image.h"
int game_hooks_configure(ElfImage *image);
void *game_glyph(void *action);
void game_settings_apply_launcher(void);
#endif
