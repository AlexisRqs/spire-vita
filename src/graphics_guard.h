#ifndef GRAPHICS_GUARD_H
#define GRAPHICS_GUARD_H
#include <stdint.h>
#include "compat_libc.h"
#define GRAPHICS_GUARD_SLOTS 256
/* Configure before resolving any guest graphics entrypoints. */
int graphics_guard_configure(int enabled);
int graphics_guard_ready(void);
uintptr_t graphics_guard_wrap(const char *name,uintptr_t target,int covered);
uintptr_t ANDROID_ABI graphics_guard_hit(unsigned index);
/* Adapter callback: invalidate an active capture and any earlier reusable stream. */
void graphics_grid_unknown_call(const char *name);
extern uintptr_t graphics_guard_entries[GRAPHICS_GUARD_SLOTS];
#endif
