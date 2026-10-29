#include "audio_runtime.h"
#include "audio_sources.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#ifdef __vita__
#include <psp2/kernel/processmgr.h>
#else
#include <time.h>
#endif
static uint64_t prepare_clock_us(void) {
#ifdef __vita__
    return sceKernelGetProcessTimeWide();
#else
    struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;
    return (uint64_t)t.tv_sec*1000000+(uint64_t)t.tv_nsec/1000;
#endif
}
#define MUSIC_COUNT 64
#define SOUND_COUNT 512
#define MUSIC_TAG 0xd5000000u
#define SOUND_TAG 0xd6000000u
#define VOICE_TAG 0x57000000u
#define SERIAL_MAX 0xffffffu

typedef struct {uint32_t token,voice;float volume;int loop;char path[256];} Music;
typedef struct {uint32_t token;int active,music,loop;float volume,pitch,pan;char path[256];} Request;
enum LoadState { LOAD_EMPTY, LOAD_OPENING, LOAD_READY, LOAD_PLAYING, LOAD_RETIRED };
typedef struct {
    struct AudioRuntime *owner;
    unsigned index;
    enum LoadState state;
    uint32_t token;
    AudioMixSource source;
} PreparedSource;
struct AudioRuntime {
    pthread_mutex_t lock;
    pthread_cond_t prepare_wake;
    pthread_t preparer;
    int prepare_quit;
    PreparedSource prepared[32];
    AudioSources sources;
    AudioMix mix;
    Music music[MUSIC_COUNT];
    char sounds[SOUND_COUNT][256];
    unsigned sound_count;
    uint32_t music_serial,voice_serial;
    Request requests[32];
    uint32_t live_token[32],live_voice[32]; /* worker only */
    int paused;
    AudioRuntimeStats stats;
};
static Music *find_music(AudioRuntime *r,uint32_t id) {
    if(!id)return NULL;
    for(unsigned i=0;i<MUSIC_COUNT;++i)if(r->music[i].token==id)return &r->music[i];
    return NULL;
}
static Request *find_voice(AudioRuntime *r,uint32_t id) {
    if(!id)return NULL;
    for(unsigned i=0;i<32;++i)if(r->requests[i].token==id && r->requests[i].active)return &r->requests[i];
    return NULL;
}
static Request *resolve(AudioRuntime *r,uint32_t id) {
    Music *m=find_music(r,id);return find_voice(r,m?m->voice:id);
}
static int path_ok(const char *path,int music) {
    if(!path || strlen(path)>=256)return 0;
    const char *prefix=music?"audio/music/":"audio/sound/";
    if(strncmp(path,prefix,strlen(prefix)))return 0;
    const char *start=path;
    for(const char *p=path;;++p){
        if(*p==':' || *p=='\\' || (*p && (unsigned char)*p<32))return 0;
        if(!*p || *p=='/'){
            size_t n=(size_t)(p-start);
            if(!n || (n==1 && *start=='.') || (n==2 && start[0]=='.' && start[1]=='.'))return 0;
            if(!*p)return 1;
            start=p+1;
        }
    }
}
/* The preparer owns pool allocation and all file close operations. A prepared
 * decoder transfers to the audio worker only in LOAD_PLAYING. Retirement
 * transfers ownership back under lock; nobody touches its decoder meanwhile. */
static void *prepare_worker(void *context) {
    AudioRuntime *r=context;
    pthread_mutex_lock(&r->lock);
    for(;;) {
        int selected=-1,closing=0;
        for(unsigned i=0;i<32;++i) {
            PreparedSource *p=&r->prepared[i];Request *q=&r->requests[i];
            if(p->state==LOAD_RETIRED || (p->state==LOAD_READY &&
               (r->prepare_quit || !q->active || q->token!=p->token))) {
                selected=(int)i;closing=1;break;
            }
        }
        if(selected<0 && r->prepare_quit)break;
        if(selected<0 && !r->paused)for(unsigned i=0;i<32;++i) {
            if(r->prepared[i].state==LOAD_EMPTY && r->requests[i].active){selected=(int)i;break;}
        }
        if(selected<0){pthread_cond_wait(&r->prepare_wake,&r->lock);continue;}
        PreparedSource *p=&r->prepared[selected];
        if(closing) {
            AudioMixSource source=p->source;p->state=LOAD_OPENING;
            pthread_mutex_unlock(&r->lock);source.close(source.context);pthread_mutex_lock(&r->lock);
            memset(&p->source,0,sizeof p->source);p->state=LOAD_EMPTY;continue;
        }
        Request request=r->requests[selected];p->token=request.token;p->state=LOAD_OPENING;
        pthread_mutex_unlock(&r->lock);
        uint64_t begin=prepare_clock_us();
        AudioMixSource source;int opened=audio_sources_open(&r->sources,request.path,request.music,&source);
        uint64_t elapsed=prepare_clock_us()-begin;
        pthread_mutex_lock(&r->lock);
        r->stats.prepare_us+=elapsed;
        if(elapsed>r->stats.max_prepare_us)r->stats.max_prepare_us=elapsed;
        if(opened){
            p->source=source;
            p->state=(!r->prepare_quit && r->requests[selected].active &&
                      r->requests[selected].token==request.token)?LOAD_READY:LOAD_RETIRED;
            ++r->stats.prepared;
        }else{
            p->state=LOAD_EMPTY;
            if(r->requests[selected].token==request.token)r->requests[selected].active=0;
            ++r->stats.failed_starts;
        }
    }
    pthread_mutex_unlock(&r->lock);return NULL;
}
static size_t prepared_read(void *context,int16_t *out,size_t frames) {
    PreparedSource *p=context;return p->source.read(p->source.context,out,frames);
}
static int prepared_seek(void *context,uint32_t frame) {
    PreparedSource *p=context;return p->source.seek(p->source.context,frame);
}
static void prepared_close(void *context) {
    PreparedSource *p=context;AudioRuntime *r=p->owner;
    pthread_mutex_lock(&r->lock);
    if(p->state==LOAD_PLAYING){
        p->state=LOAD_RETIRED;
        /* EOF and explicit stop both retire this generation. Prevent preparer
         * from reopening it in the interval before render's completion pass. */
        if(r->requests[p->index].token==p->token)r->requests[p->index].active=0;
        pthread_cond_signal(&r->prepare_wake);
    }
    pthread_mutex_unlock(&r->lock);
}
AudioRuntime *audio_runtime_create(const char *root,uint32_t rate) {
    AudioRuntime *r=calloc(1,sizeof *r);if(!r)return NULL;
    if(!audio_sources_init(&r->sources,root) || !audio_mix_init(&r->mix,rate)) {free(r);return NULL;}
    if(pthread_mutex_init(&r->lock,NULL)){free(r);return NULL;}
    if(pthread_cond_init(&r->prepare_wake,NULL)){pthread_mutex_destroy(&r->lock);free(r);return NULL;}
    for(unsigned i=0;i<32;++i){r->prepared[i].owner=r;r->prepared[i].index=i;}
    pthread_attr_t attr;
    if(pthread_attr_init(&attr))goto fail;
    int result=pthread_attr_setstacksize(&attr,128*1024);
    if(!result)result=pthread_create(&r->preparer,&attr,prepare_worker,r);
    pthread_attr_destroy(&attr);
    if(result)goto fail;
    return r;
fail:
    pthread_cond_destroy(&r->prepare_wake);pthread_mutex_destroy(&r->lock);free(r);return NULL;
}
void audio_runtime_destroy(AudioRuntime *r) {
    if(!r)return;
    audio_mix_shutdown(&r->mix);
    pthread_mutex_lock(&r->lock);r->prepare_quit=1;pthread_cond_signal(&r->prepare_wake);pthread_mutex_unlock(&r->lock);
    pthread_join(r->preparer,NULL);
    pthread_cond_destroy(&r->prepare_wake);pthread_mutex_destroy(&r->lock);free(r);
}
uint32_t audio_runtime_asset(AudioRuntime *r,const char *path,int music) {
    if(!r || !path_ok(path,music))return 0;
    uint32_t result=0;pthread_mutex_lock(&r->lock);
    if(music) {
        for(unsigned i=0;i<MUSIC_COUNT && r->music_serial<SERIAL_MAX;++i)if(!r->music[i].token) {
            Music *m=&r->music[i];memset(m,0,sizeof *m);m->token=MUSIC_TAG|++r->music_serial;
            m->volume=1;strcpy(m->path,path);result=m->token;break;
        }
    } else {
        for(unsigned i=0;i<r->sound_count;++i)if(!strcmp(r->sounds[i],path)){result=SOUND_TAG|(i+1);break;}
        if(!result && r->sound_count<SOUND_COUNT){strcpy(r->sounds[r->sound_count++],path);result=SOUND_TAG|r->sound_count;}
    }
    pthread_mutex_unlock(&r->lock);return result;
}
static uint32_t start(AudioRuntime *r,const char *path,int music,float volume,float pitch,float pan,int loop) {
    if(!isfinite(volume)||!isfinite(pitch)||!isfinite(pan)||volume<0||volume>1||pitch<.125f||pitch>8||pan< -1||pan>1||r->voice_serial==SERIAL_MAX) {
        ++r->stats.rejected_starts;return 0;
    }
    unsigned begin=music?0:4,end=music?4:32;
    for(unsigned i=begin;i<end;++i)if(!r->requests[i].active) {
        Request *q=&r->requests[i];memset(q,0,sizeof *q);
        q->token=VOICE_TAG|++r->voice_serial;q->active=1;q->music=music;q->volume=volume;
        q->pitch=pitch;q->pan=pan;q->loop=!!loop;strcpy(q->path,path);pthread_cond_signal(&r->prepare_wake);return q->token;
    }
    ++r->stats.rejected_starts;return 0;
}
uint32_t audio_runtime_sound_play(AudioRuntime *r,uint32_t asset,float vol,float pitch,float pan,int loop) {
    if(!r)return 0;
    uint32_t result=0;pthread_mutex_lock(&r->lock);
    unsigned index=asset&SERIAL_MAX;
    if((asset&0xff000000u)==SOUND_TAG && index && index<=r->sound_count)
        result=start(r,r->sounds[index-1],0,vol,pitch,pan,loop);
    pthread_mutex_unlock(&r->lock);return result;
}
void audio_runtime_music_play(AudioRuntime *r,uint32_t id) {
    if(!r)return;
    pthread_mutex_lock(&r->lock);Music *m=find_music(r,id);
    if(m && !find_voice(r,m->voice))m->voice=start(r,m->path,1,m->volume,1,0,m->loop);
    pthread_mutex_unlock(&r->lock);
}
void audio_runtime_dispose_music(AudioRuntime *r,uint32_t id) {
    if(!r)return;
    pthread_mutex_lock(&r->lock);Music *m=find_music(r,id);
    if(m){Request *q=find_voice(r,m->voice);if(q)q->active=0;memset(m,0,sizeof *m);pthread_cond_signal(&r->prepare_wake);}
    pthread_mutex_unlock(&r->lock);
}
void audio_runtime_stop(AudioRuntime *r,uint32_t id) {
    if(!r)return;
    pthread_mutex_lock(&r->lock);Request *q=resolve(r,id);if(q)q->active=0;pthread_cond_signal(&r->prepare_wake);pthread_mutex_unlock(&r->lock);
}
void audio_runtime_volume(AudioRuntime *r,uint32_t id,float volume) {
    if(!r || !isfinite(volume))return;
    if(volume<0)volume=0;
    if(volume>1)volume=1;
    pthread_mutex_lock(&r->lock);Music *m=find_music(r,id);if(m)m->volume=volume;
    Request *q=resolve(r,id);if(q)q->volume=volume;pthread_mutex_unlock(&r->lock);
}
float audio_runtime_get_volume(AudioRuntime *r,uint32_t id) {
    if(!r)return 0;
    pthread_mutex_lock(&r->lock);Music *m=find_music(r,id);float value=m?m->volume:0;pthread_mutex_unlock(&r->lock);return value;
}
void audio_runtime_loop(AudioRuntime *r,uint32_t id,int loop) {
    if(!r)return;
    pthread_mutex_lock(&r->lock);Music *m=find_music(r,id);if(m)m->loop=!!loop;
    Request *q=resolve(r,id);if(q)q->loop=!!loop;pthread_mutex_unlock(&r->lock);
}
int audio_runtime_playing(AudioRuntime *r,uint32_t id) {
    if(!r)return 0;
    pthread_mutex_lock(&r->lock);int playing=resolve(r,id)!=NULL;pthread_mutex_unlock(&r->lock);return playing;
}
void audio_runtime_pause(AudioRuntime *r,int pause) {if(r){pthread_mutex_lock(&r->lock);r->paused=!!pause;pthread_cond_signal(&r->prepare_wake);pthread_mutex_unlock(&r->lock);}}
static void finish(AudioRuntime *r,unsigned i,uint32_t token,int failed) {
    pthread_mutex_lock(&r->lock);
    if(r->requests[i].token==token)r->requests[i].active=0;
    if(failed)++r->stats.failed_starts;else ++r->stats.completed;
    pthread_mutex_unlock(&r->lock);
}
void audio_runtime_render(AudioRuntime *r,int16_t *pcm,size_t frames) {
    if(!r || !pcm)return;
    Request snapshot[32];pthread_mutex_lock(&r->lock);
    memcpy(snapshot,r->requests,sizeof snapshot);int pause=r->paused;pthread_mutex_unlock(&r->lock);
    audio_mix_pause(&r->mix,pause);
    for(unsigned i=0;i<32;++i) {
        Request *q=&snapshot[i];
        if(r->live_voice[i] && (!q->active || q->token!=r->live_token[i])) {
            audio_mix_stop(&r->mix,r->live_voice[i]);r->live_voice[i]=0;
        }
        if(!q->active)continue;
        if(!r->live_voice[i]) {
            if(pause)continue;
            AudioMixSource source;PreparedSource *prepared=&r->prepared[i];
            pthread_mutex_lock(&r->lock);
            int current=r->requests[i].token==q->token && r->requests[i].active &&
                        prepared->state==LOAD_READY && prepared->token==q->token;
            if(current){
                *q=r->requests[i];prepared->state=LOAD_PLAYING;
                source=prepared->source;source.context=prepared;source.read=prepared_read;
                source.seek=prepared_seek;source.close=prepared_close;
            }
            pthread_mutex_unlock(&r->lock);
            if(!current)continue;
            uint32_t voice=audio_mix_start(&r->mix,&source,q->volume,q->pitch,q->pan,q->loop);
            if(!voice){source.close(source.context);finish(r,i,q->token,1);continue;}
            r->live_voice[i]=voice;r->live_token[i]=q->token;
            pthread_mutex_lock(&r->lock);++r->stats.starts;pthread_mutex_unlock(&r->lock);
        } else audio_mix_control(&r->mix,r->live_voice[i],q->volume,q->pitch,q->pan,q->loop);
    }
    audio_mix_render(&r->mix,pcm,frames);
    for(unsigned i=0;i<32;++i)if(r->live_voice[i] && !audio_mix_playing(&r->mix,r->live_voice[i])) {
        finish(r,i,r->live_token[i],0);r->live_voice[i]=0;
    }
    pthread_mutex_lock(&r->lock);r->stats.source_errors=r->mix.source_errors;r->stats.clipped_samples=r->mix.clipped_samples;pthread_mutex_unlock(&r->lock);
}
AudioRuntimeStats audio_runtime_stats(AudioRuntime *r) {
    AudioRuntimeStats s={0};if(!r)return s;
    pthread_mutex_lock(&r->lock);s=r->stats;
    for(unsigned i=0;i<32;++i){s.active+=!!r->requests[i].active;s.ready+=r->prepared[i].state==LOAD_READY;s.loader_busy+=r->prepared[i].state==LOAD_OPENING;}
    pthread_mutex_unlock(&r->lock);return s;
}
