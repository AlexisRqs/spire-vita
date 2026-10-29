#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H
#include <stdint.h>
void loading_screen_draw(uint32_t *pixels,const unsigned char *font,const char *stage,unsigned elapsed_ms);
void launcher_screen_draw(uint32_t*,const unsigned char*,int,int,int,int,int,const char*);
void launcher_loading_status_draw(uint32_t*,const unsigned char*,int,int,int,int,const char*,unsigned,unsigned);
int boot_display_start(void);
void boot_display_message(const char *message);
void boot_display_stop(void);
void boot_display_release(void);
#endif
