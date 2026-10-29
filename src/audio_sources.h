#ifndef AUDIO_SOURCES_H
#define AUDIO_SOURCES_H
#include "audio_mix.h"
#include "audio_wave.h"
#include "audio_vorbis.h"
#define AUDIO_SOURCE_MUSIC 4
#define AUDIO_SOURCE_EFFECTS 28
#define AUDIO_SOURCE_ARENA (256u*1024u)
typedef struct {
    int active, music;
    FILE *file;
    void *arena;
    union {AudioWave wave;AudioVorbis vorbis;} decoder;
    char io_buffer[4096];
} AudioFileSource;
typedef struct {
    char root[1024];
    AudioFileSource sources[AUDIO_SOURCE_MUSIC+AUDIO_SOURCE_EFFECTS];
    uint64_t open_errors, pool_full;
} AudioSources;
/* Worker-only pool. Init only a fresh pool; root is trusted assets directory.
 * Do not reinit before closing all outstanding sources. */
int audio_sources_init(AudioSources *,const char *root);
/* path is a relative asset path (audio/music/... or audio/sound/...).
 * Caller owns returned source until mixer accepts it; on mixer failure call
 * source.close(context). On success mixer owns it and releases pool slot. */
int audio_sources_open(AudioSources *,const char *path,int music,AudioMixSource *out);
unsigned audio_sources_active(const AudioSources *);
#endif
