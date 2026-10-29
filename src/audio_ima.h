#ifndef AUDIO_IMA_H
#define AUDIO_IMA_H
#include <stddef.h>
#include <stdint.h>
/* Decode one complete 4-bit WAV IMA block, mono/stereo, to interleaved PCM16.
 * No allocation or retained state. Returns frames, or zero on invalid input.
 * Capacity is in int16_t samples. Invalid input leaves output untouched.
 * Source and destination must not overlap. */
size_t audio_ima_decode_block(const uint8_t *src, size_t bytes,
                             unsigned channels, int16_t *dst, size_t capacity);
#endif
