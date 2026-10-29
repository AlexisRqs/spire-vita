#ifndef AUDIO_MIX_H
#define AUDIO_MIX_H
#include <stddef.h>
#include <stdint.h>
#define AUDIO_MIX_VOICES 32
#define AUDIO_MIX_BLOCK 256
/* All calls, including controls, run on one audio worker. This core is not
 * thread safe. The integration layer must queue commands from game threads.
 * Sources supply exact known lengths and interleaved signed PCM16. */
typedef struct {
    void *context;
    size_t (*read)(void *, int16_t *, size_t frames);
    int (*seek)(void *, uint32_t frame);
    void (*close)(void *);
    uint32_t frames, rate;
    unsigned channels;
} AudioMixSource;
typedef struct {
    AudioMixSource source;
    uint32_t id, buffered_start, source_cursor;
    unsigned buffered_frames;
    uint64_t phase, step;
    int gain_l, gain_r, loop;
    int first_l,first_r; /* loop lookahead without replacing buffered tail */
    int16_t buffer[(AUDIO_MIX_BLOCK+1)*2];
} AudioMixVoice;
typedef struct {
    AudioMixVoice voices[AUDIO_MIX_VOICES];
    uint32_t serial, rate;
    int paused;
    uint64_t source_errors, rejected_starts, clipped_samples;
} AudioMix;
int audio_mix_init(AudioMix *, uint32_t output_rate);
/* Success transfers source ownership to mixer, failure leaves it to caller.
 * Source must be positioned at frame zero. Internal IDs require translation
 * to engine ABI handles; they are not engine pointers or OpenAL IDs.
 * Volume [0,1], pitch [0.125,8], pan [-1,1], all finite. id 0 means failure.
 * No eviction; a full pool rejects the new voice. IDs are never reused. */
uint32_t audio_mix_start(AudioMix *, const AudioMixSource *, float volume, float pitch, float pan, int loop);
int audio_mix_control(AudioMix *, uint32_t id, float volume, float pitch, float pan, int loop);
int audio_mix_playing(const AudioMix *, uint32_t id);
void audio_mix_stop(AudioMix *, uint32_t id);
void audio_mix_shutdown(AudioMix *);
/* Pause outputs silence without consuming sources or advancing voices. */
void audio_mix_pause(AudioMix *, int paused);
/* Produces stereo PCM16, silence when idle; no allocation. */
void audio_mix_render(AudioMix *, int16_t *stereo, size_t frames);
#endif
