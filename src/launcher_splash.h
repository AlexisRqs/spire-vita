#ifndef LAUNCHER_SPLASH_H
#define LAUNCHER_SPLASH_H
#include <stdint.h>
#define LAUNCHER_SPLASH_DURATION_MS 4000u
int launcher_splash_prepare(void);
void launcher_splash_draw(uint32_t *pixels,unsigned elapsed_ms);
void launcher_splash_release(void);
#endif
