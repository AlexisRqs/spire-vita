#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H
#include "audio_runtime.h"
typedef struct AudioOutput AudioOutput;
/* Runtime must outlive output. Owns port, decoder producer and output worker.
 * Four queued 512-frame blocks add up to 42.7ms of PCM lookahead, plus a block
 * being produced and the output device buffers. Stop joins both workers. */
AudioOutput *audio_output_start(AudioRuntime *);
/* Call after stopping game callers, before destroying runtime. Joins worker. */
void audio_output_stop(AudioOutput *);
typedef struct {uint64_t blocks,max_render_us,slow_blocks,elapsed_us,render_us;
 uint64_t underruns,queue_wait_us,max_queue_wait_us,produced;
 unsigned queued;int error,running;} AudioOutputStats;
AudioOutputStats audio_output_stats(AudioOutput *);
#endif
