#ifndef LAUNCHER_BACKGROUND_H
#define LAUNCHER_BACKGROUND_H
#include <stdint.h>
void launcher_background_prepare(void);
void launcher_background_cancel(void);
int launcher_background_active(void);
void launcher_background_tick(void);
void launcher_background_draw(uint32_t *pixels,const unsigned char *font);
void launcher_background_phase(const char *phase);
void launcher_background_ready(void);
void launcher_background_stage(unsigned stage,const char *label);
int launcher_background_needs_frame(void);
void launcher_background_frame_ready(void);
/* Engine thread only. Reveals immediately; blocks game input until the launch button/front touch is released. */
int launcher_background_gate(int held);
#endif
