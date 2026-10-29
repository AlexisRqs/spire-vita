#ifndef AUDIO_RUNTIME_H
#define AUDIO_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
typedef struct AudioRuntime AudioRuntime;
/* Create before publishing to game; destroy only after worker joined and all
 * game callers stopped. Exactly one audio worker calls render; an internal preparer opens/closes files. Other APIs are locked
 * and do no file I/O. All handles are opaque tagged uint32 values, never ptrs. */
AudioRuntime *audio_runtime_create(const char *asset_root,uint32_t output_rate);
void audio_runtime_destroy(AudioRuntime *);
uint32_t audio_runtime_asset(AudioRuntime *,const char *path,int music);
uint32_t audio_runtime_sound_play(AudioRuntime *,uint32_t asset,float volume,float pitch,float pan,int loop);
void audio_runtime_music_play(AudioRuntime *,uint32_t music);
void audio_runtime_dispose_music(AudioRuntime *,uint32_t music);
void audio_runtime_stop(AudioRuntime *,uint32_t handle);
void audio_runtime_volume(AudioRuntime *,uint32_t handle,float volume);
float audio_runtime_get_volume(AudioRuntime *,uint32_t music);
void audio_runtime_loop(AudioRuntime *,uint32_t handle,int loop);
int audio_runtime_playing(AudioRuntime *,uint32_t handle);
void audio_runtime_pause(AudioRuntime *,int pause);
void audio_runtime_render(AudioRuntime *,int16_t *stereo,size_t frames);
typedef struct {uint64_t starts,failed_starts,rejected_starts,completed,source_errors,clipped_samples,prepared,prepare_us,max_prepare_us;unsigned active,ready,loader_busy;} AudioRuntimeStats;
AudioRuntimeStats audio_runtime_stats(AudioRuntime *);
#endif
