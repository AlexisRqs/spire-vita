#include "audio_output.h"
#include <psp2/audioout.h>
#include <psp2/kernel/processmgr.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#define OUTPUT_FRAMES 512
#define QUEUE_BLOCKS 4
struct AudioOutput {
    AudioRuntime *runtime;
    pthread_t thread,producer;
    pthread_mutex_t lock;
    pthread_cond_t changed;
    int port,quit;
    unsigned head,count;
    AudioOutputStats stats;
    int16_t queue[QUEUE_BLOCKS][OUTPUT_FRAMES*2];
    int16_t pcm[2][OUTPUT_FRAMES*2];
};
static void *produce_worker(void *ctx) {
    AudioOutput *o=ctx;
    int16_t pcm[OUTPUT_FRAMES*2];
    for(;;) {
        pthread_mutex_lock(&o->lock);
        while(!o->quit && o->count==QUEUE_BLOCKS)pthread_cond_wait(&o->changed,&o->lock);
        int quit=o->quit;pthread_mutex_unlock(&o->lock);
        if(quit)break;
        uint64_t begin=sceKernelGetProcessTimeWide();
        audio_runtime_render(o->runtime,pcm,OUTPUT_FRAMES);
        uint64_t elapsed=sceKernelGetProcessTimeWide()-begin;
        pthread_mutex_lock(&o->lock);
        o->stats.render_us+=elapsed;
        if(elapsed>o->stats.max_render_us)o->stats.max_render_us=elapsed;
        if(elapsed>10666)++o->stats.slow_blocks;
        if(!o->quit) {
            /* One producer, so the free slot cannot be taken during render. */
            memcpy(o->queue[(o->head+o->count)%QUEUE_BLOCKS],pcm,sizeof pcm);
            ++o->count;++o->stats.produced;pthread_cond_broadcast(&o->changed);
        }
        quit=o->quit;pthread_mutex_unlock(&o->lock);
        if(quit)break;
    }
    return NULL;
}
static void *output_worker(void *ctx) {
    AudioOutput *o=ctx;unsigned buffer=0;int primed=0;
    uint64_t started=0;
    for(;;) {
        pthread_mutex_lock(&o->lock);
        /* Fill once before playback. After starvation, resume as soon as one
         * block arrives; never replay stale PCM or silently drop queued audio. */
        unsigned needed=primed?1:QUEUE_BLOCKS;
        uint64_t wait_start=0;
        if(primed && !o->count && !o->quit){++o->stats.underruns;wait_start=sceKernelGetProcessTimeWide();}
        while(!o->quit && o->count<needed)pthread_cond_wait(&o->changed,&o->lock);
        if(wait_start){
            uint64_t waited=sceKernelGetProcessTimeWide()-wait_start;
            o->stats.queue_wait_us+=waited;
            if(waited>o->stats.max_queue_wait_us)o->stats.max_queue_wait_us=waited;
        }
        if(o->quit){pthread_mutex_unlock(&o->lock);break;}
        /* Device may retain the previous pointer. Copy to alternating output
         * buffers before returning queue ownership to the producer. */
        memcpy(o->pcm[buffer],o->queue[o->head],sizeof o->pcm[buffer]);
        o->head=(o->head+1)%QUEUE_BLOCKS;--o->count;
        pthread_cond_broadcast(&o->changed);pthread_mutex_unlock(&o->lock);
        if(!primed){started=sceKernelGetProcessTimeWide();primed=1;}
        int result=sceAudioOutOutput(o->port,o->pcm[buffer]);
        pthread_mutex_lock(&o->lock);
        o->stats.elapsed_us=sceKernelGetProcessTimeWide()-started;
        if(result<0){o->stats.error=result;o->quit=1;pthread_cond_broadcast(&o->changed);}
        else ++o->stats.blocks;
        pthread_mutex_unlock(&o->lock);
        if(result<0)break;
        buffer^=1;
    }
    int drain=sceAudioOutOutput(o->port,NULL);
    pthread_mutex_lock(&o->lock);
    if(drain<0 && !o->stats.error)o->stats.error=drain;
    o->stats.running=0;pthread_mutex_unlock(&o->lock);
    return NULL;
}
AudioOutput *audio_output_start(AudioRuntime *runtime) {
    if(!runtime)return NULL;
    AudioOutput *o=calloc(1,sizeof *o);if(!o)return NULL;o->runtime=runtime;
    if(pthread_mutex_init(&o->lock,NULL)){free(o);return NULL;}
    if(pthread_cond_init(&o->changed,NULL)){pthread_mutex_destroy(&o->lock);free(o);return NULL;}
    o->port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,OUTPUT_FRAMES,48000,SCE_AUDIO_OUT_MODE_STEREO);
    if(o->port<0)goto fail;
    int volume[2]={SCE_AUDIO_OUT_MAX_VOL,SCE_AUDIO_OUT_MAX_VOL};
    if(sceAudioOutSetVolume(o->port,SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH,volume)<0)goto port_fail;
    pthread_attr_t attr;
    if(pthread_attr_init(&attr))goto port_fail;
    int result=pthread_attr_setstacksize(&attr,128*1024);
    if(!result)result=pthread_create(&o->producer,&attr,produce_worker,o);
    if(result){pthread_attr_destroy(&attr);goto port_fail;}
    o->stats.running=1;
    result=pthread_create(&o->thread,&attr,output_worker,o);
    pthread_attr_destroy(&attr);
    if(result){
        pthread_mutex_lock(&o->lock);o->quit=1;pthread_cond_broadcast(&o->changed);pthread_mutex_unlock(&o->lock);
        pthread_join(o->producer,NULL);goto port_fail;
    }
    return o;
port_fail:
    sceAudioOutReleasePort(o->port);
fail:
    pthread_cond_destroy(&o->changed);pthread_mutex_destroy(&o->lock);free(o);return NULL;
}
void audio_output_stop(AudioOutput *o) {
    if(!o)return;
    pthread_mutex_lock(&o->lock);o->quit=1;pthread_cond_broadcast(&o->changed);pthread_mutex_unlock(&o->lock);
    pthread_join(o->thread,NULL);pthread_join(o->producer,NULL);
    sceAudioOutReleasePort(o->port);
    pthread_cond_destroy(&o->changed);pthread_mutex_destroy(&o->lock);free(o);
}
AudioOutputStats audio_output_stats(AudioOutput *o) {
    AudioOutputStats s={0};if(!o)return s;
    pthread_mutex_lock(&o->lock);s=o->stats;s.queued=o->count;pthread_mutex_unlock(&o->lock);return s;
}
