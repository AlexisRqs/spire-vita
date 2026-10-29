#ifndef SCENE_METRICS_H
#define SCENE_METRICS_H
#include <stdint.h>
uint64_t scene_clock(void);
unsigned scene_begin(const char *name);
void scene_return(unsigned token);
void scene_present(void);
void scene_asset_io(uint64_t us,unsigned bytes);
#endif
