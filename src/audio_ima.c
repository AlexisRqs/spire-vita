#include "audio_ima.h"

/* IMA ADPCM standard quantizer step sizes and index adjustments. */
static const int steps[89] = {
 7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,
 60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,
 337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,
 1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,
 4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,
 13899,15289,16818,18500,20350,22385,24623,27086,29794,32767
};
static const int adjustments[8] = {-1,-1,-1,-1,2,4,6,8};

size_t audio_ima_decode_block(const uint8_t *src, size_t bytes,
                             unsigned channels, int16_t *dst, size_t capacity) {
    if (!src || !dst || channels < 1 || channels > 2) return 0;
    const size_t header = 4 * channels;
    if (bytes < header || (bytes - header) % header) return 0;
    const size_t groups = (bytes - header) / header;
    if (groups > (SIZE_MAX / channels - 1) / 8) return 0;
    const size_t frames = 1 + groups * 8;
    if (capacity < frames * channels) return 0;
    int predictor[2], index[2];
    for (unsigned c = 0; c < channels; ++c) {
        if (src[4*c+2] > 88 || src[4*c+3] != 0) return 0;
        unsigned raw = src[4*c] | ((unsigned)src[4*c+1] << 8);
        predictor[c] = raw >= 32768 ? (int)raw - 65536 : (int)raw;
        index[c] = src[4*c+2];
    }
    for (unsigned c = 0; c < channels; ++c) dst[c] = (int16_t)predictor[c];
    for (size_t g = 0; g < groups; ++g) {
        for (unsigned c = 0; c < channels; ++c) {
            const uint8_t *packed = src + header + g * header + c * 4;
            for (unsigned n = 0; n < 8; ++n) {
                unsigned code = (packed[n/2] >> ((n%2)*4)) & 15;
                int step = steps[index[c]];
                int magnitude = (step >> 3) + ((code & 1) ? step >> 2 : 0)
                              + ((code & 2) ? step >> 1 : 0)
                              + ((code & 4) ? step : 0);
                int value = predictor[c] + ((code & 8) ? -magnitude : magnitude);
                predictor[c] = value < -32768 ? -32768 : value > 32767 ? 32767 : value;
                int next = index[c] + adjustments[code & 7];
                index[c] = next < 0 ? 0 : next > 88 ? 88 : next;
                dst[(1+g*8+n)*channels+c] = (int16_t)predictor[c];
            }
        }
    }
    return frames;
}
