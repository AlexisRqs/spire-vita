#ifndef AUDIO_VORBIS_H
#define AUDIO_VORBIS_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
/* Single-worker ownership. Caller owns aligned arena for entire decoder lifetime.
 * On success this object owns FILE; on failure caller still owns FILE.
 * FILE starts at offset zero, contains one complete Ogg Vorbis stream, and must
 * not be read/seeked elsewhere while open. No whole-file allocation. */
typedef struct {
    void *decoder;
    FILE *file;
    uint32_t frames, rate, position;
    unsigned channels, setup_bytes, setup_temp_bytes, decode_temp_bytes;
    int failed, error;
} AudioVorbis;
int audio_vorbis_open(AudioVorbis *, FILE *, void *arena, size_t arena_bytes);
size_t audio_vorbis_read(AudioVorbis *, int16_t *, size_t frames);
int audio_vorbis_seek(AudioVorbis *, uint32_t frame);
void audio_vorbis_close(AudioVorbis *);
#endif
