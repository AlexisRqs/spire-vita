#ifndef AUDIO_WAVE_H
#define AUDIO_WAVE_H
#include "audio_ima.h"
/* Positional reader must return exactly count on success; shorter reads fail.
 * Context and stream belong to one worker; not safe for concurrent access.
 * No FILE ownership or allocations are hidden in this interface. */
typedef size_t (*AudioReadAt)(void *context, uint64_t offset, void *dst, size_t count);
typedef struct {
    AudioReadAt read_at;
    void *context;
    uint64_t data_offset, position;
    uint32_t frames, data_bytes, rate;
    unsigned channels, block_bytes, block_frames;
    uint64_t cached_block;
    unsigned cached_frames;
    int failed;
    uint8_t compressed[1024];
    int16_t pcm[2048];
} AudioWave;
/* Supports complete IMA WAV blocks <=1024 bytes, mono/stereo, 4 bits.
 * Requires a fact length; rejects unsupported formats rather than guessing.
 * Failure clears state. Physical file length must be supplied by the caller. */
int audio_wave_open(AudioWave *s, AudioReadAt reader, void *context, uint64_t bytes);
/* Returns frames read; zero is EOF unless s->failed. Output is interleaved.
 * Caller supplies capacity for requested * s->channels samples. */
size_t audio_wave_read(AudioWave *s, int16_t *out, size_t requested);
/* Frame-accurate repositioning, including EOF; use seek(0) to loop. */
int audio_wave_seek(AudioWave *s, uint64_t frame);
#endif
