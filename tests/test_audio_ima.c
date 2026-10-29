#include "audio_ima.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void rejected(const unsigned char *data, size_t bytes, unsigned channels,
                     size_t capacity) {
    int16_t output[2034], before[2034];
    memset(output, 0x5a, sizeof output);
    memcpy(before, output, sizeof output);
    assert(audio_ima_decode_block(data, bytes, channels, output, capacity) == 0);
    assert(!memcmp(output, before, sizeof output));
}
int main(void) {
    unsigned char block[1024] = {0};
    int16_t output[2034];
    assert(audio_ima_decode_block(block, 1024, 2, output, 2034) == 1017);
    for (unsigned i = 0; i < 2034; ++i) assert(output[i] == 0);
    rejected(NULL, 1024, 2, 2034);
    rejected(block, 1024, 0, 2034);
    rejected(block, 1024, 3, 2034);
    rejected(block, 1024, 2, 2033);
    assert(audio_ima_decode_block(block, 1024, 2, NULL, 2034) == 0);
    for (size_t n = 0; n < 1024; ++n) {
        if (n < 8 || n % 8) rejected(block, n, 2, 2034);
    }
    for (unsigned c = 0; c < 2; ++c) {
        for (unsigned index = 89; index <= 255; ++index) {
            block[4*c+2] = index;
            rejected(block, 1024, 2, 2034);
        }
        block[4*c+2] = 0;
        block[4*c+3] = 1;
        rejected(block, 1024, 2, 2034);
        block[4*c+3] = 0;
    }
    /* Header-only block and signed predictor handling. */
    block[0] = 0; block[1] = 128; block[4] = 255; block[5] = 127;
    assert(audio_ima_decode_block(block, 8, 2, output, 2) == 1);
    assert(output[0] == -32768 && output[1] == 32767);
    block[2] = block[6] = 88;
    memset(block+8, 0x77, 1016);
    assert(audio_ima_decode_block(block, 1024, 2, output, 2034) == 1017);
    assert(output[2032] == 32767 && output[2033] == 32767);
    memset(block+8, 0xff, 1016);
    assert(audio_ima_decode_block(block, 1024, 2, output, 2034) == 1017);
    assert(output[2032] == -32768 && output[2033] == -32768);
    /* Mono uses the same four-byte groups, independently of stereo. */
    memset(block, 0, sizeof block);
    assert(audio_ima_decode_block(block, 8, 1, output, 9) == 9);
    for (unsigned i=0; i<9; ++i) assert(output[i] == 0);
    puts("audio IMA bounds, malformed headers, saturation and mono: pass");
}
