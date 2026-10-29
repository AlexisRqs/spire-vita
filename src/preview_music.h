#ifndef PREVIEW_MUSIC_H
#define PREVIEW_MUSIC_H
#include "elf_image.h"
#include "compat_libc.h"
void *preview_music_new(const char *path);
void preview_music_play(void *handle);
void preview_music_dispose(void *handle);
void preview_music_stop(void *handle);
int preview_music_playing(void *handle);
void preview_music_loop(void *handle,int loop);
void ANDROID_ABI preview_music_set_volume(void *handle,float volume);
float ANDROID_ABI preview_music_get_volume(void *handle);
void preview_music_bind(void (*stop)(void *),void (*loop)(void *,int),int (*playing)(void *));
void *preview_sound_new(const char *path);
int ANDROID_ABI preview_sound_play(void *asset,float volume);
int ANDROID_ABI preview_sound_play2(void *asset,float volume,float pitch,float pan);
void ANDROID_ABI preview_sound_volume(void *source,float volume);
int preview_music_configure(ElfImage *image);
#endif
