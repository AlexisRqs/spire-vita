#ifndef GRAPHICS_BRIDGE_H
#define GRAPHICS_BRIDGE_H
#include <stdint.h>
void graphics_recording_configure(int enabled);
uintptr_t graphics_lookup(const char *name);
int graphics_initialize(void);
int graphics_prepare_game(void);
void graphics_observe_game(int enabled);
unsigned graphics_game_frames(void);
int graphics_shader_probe(const char *asset_root);
int graphics_probe(const char *asset_root);
void graphics_grid_trace_begin(unsigned pass,int sample);
void graphics_grid_trace_end(void);
void graphics_grid_stream_begin(unsigned pass,unsigned frame,int sample);
void graphics_grid_stream_end(void);
/* Managed-state validation is mandatory before calling replay. */
int graphics_grid_replay_ready(unsigned frame);
int graphics_grid_replay(unsigned frame);
#endif
