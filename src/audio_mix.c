#include "audio_mix.h"
#include <limits.h>
#include <math.h>
#include <string.h>

static int parameters(AudioMixVoice *v,uint32_t rate,float volume,float pitch,float pan,int loop) {
    if (!isfinite(volume) || !isfinite(pitch) || !isfinite(pan) || volume<0 || volume>1 ||
        pitch<0.125f || pitch>8 || pan< -1 || pan>1) return 0;
    v->gain_l=(int)(32768.0f*volume*(pan>0?1-pan:1));
    v->gain_r=(int)(32768.0f*volume*(pan<0?1+pan:1));
    v->step=(uint64_t)((double)v->source.rate/rate*pitch*4294967296.0+0.5);
    v->loop=!!loop;
    return 1;
}
static void release(AudioMixVoice *v) {
    if (!v->id) return;
    AudioMixSource source=v->source;
    v->id=0;
    if(source.close) source.close(source.context);
    memset(&v->source,0,sizeof v->source);
}
int audio_mix_init(AudioMix *m,uint32_t rate) {
    if (!m || rate<8000 || rate>192000) return 0;
    memset(m,0,sizeof *m);m->rate=rate;return 1;
}
uint32_t audio_mix_start(AudioMix *m,const AudioMixSource *s,float volume,float pitch,float pan,int loop) {
    if (!m || !m->rate) return 0;
    if (!s || !s->read || !s->seek || !s->frames || s->frames>INT32_MAX ||
        s->rate<8000 || s->rate>192000 || (s->channels!=1 && s->channels!=2) || m->serial==UINT32_MAX) {
        ++m->rejected_starts;return 0;
    }
    for (unsigned i=0;i<AUDIO_MIX_VOICES;++i) if(!m->voices[i].id) {
        AudioMixVoice *v=&m->voices[i];memset(v,0,sizeof *v);v->source=*s;
        if(!parameters(v,m->rate,volume,pitch,pan,loop)) {++m->rejected_starts;return 0;}
        /* Source must already be positioned at frame zero on entry. */
        v->id=++m->serial;return v->id;
    }
    ++m->rejected_starts;return 0;
}
int audio_mix_control(AudioMix *m,uint32_t id,float volume,float pitch,float pan,int loop) {
    if(!m || !id)return 0;
    for(unsigned i=0;i<AUDIO_MIX_VOICES;++i)if(m->voices[i].id==id)
        return parameters(&m->voices[i],m->rate,volume,pitch,pan,loop);
    return 0;
}
int audio_mix_playing(const AudioMix *m,uint32_t id) {
    if(!m || !id)return 0;
    for(unsigned i=0;i<AUDIO_MIX_VOICES;++i)if(m->voices[i].id==id)return 1;
    return 0;
}
void audio_mix_stop(AudioMix *m,uint32_t id) {
    if(!m || !id)return;
    for(unsigned i=0;i<AUDIO_MIX_VOICES;++i)if(m->voices[i].id==id){release(&m->voices[i]);return;}
}
void audio_mix_shutdown(AudioMix *m) {
    if(!m)return;
    for(unsigned i=0;i<AUDIO_MIX_VOICES;++i)release(&m->voices[i]);
    /* Keep serial to prevent stale IDs matching after restarting voices. */
}
void audio_mix_pause(AudioMix *m,int paused) {if(m)m->paused=!!paused;}
static int sample(AudioMixVoice *v,uint32_t frame,int *left,int *right) {
    if(frame<v->buffered_start || frame-v->buffered_start>=v->buffered_frames) {
        /* Linear interpolation looks one frame ahead. Keep the old last frame
         * when filling the next block, since a fractional phase may use it
         * again. Otherwise upsampling triggers expensive backward Vorbis seeks. */
        unsigned keep=v->buffered_frames && frame==v->source_cursor &&
                      frame==v->buffered_start+v->buffered_frames;
        if(keep) {
            unsigned last=(v->buffered_frames-1)*v->source.channels;
            v->buffer[0]=v->buffer[last];
            if(v->source.channels==2)v->buffer[1]=v->buffer[last+1];
        }
        if(frame!=v->source_cursor && !v->source.seek(v->source.context,frame))return 0;
        unsigned count=v->source.frames-frame;
        if(count>AUDIO_MIX_BLOCK)count=AUDIO_MIX_BLOCK;
        if(v->source.read(v->source.context,v->buffer+keep*v->source.channels,count)!=count)return 0;
        v->buffered_start=frame-keep;v->buffered_frames=count+keep;v->source_cursor=frame+count;
    }
    unsigned offset=(frame-v->buffered_start)*v->source.channels;
    *left=v->buffer[offset];*right=v->buffer[offset+(v->source.channels==2)];
    if(!frame){v->first_l=*left;v->first_r=*right;}
    return 1;
}
void audio_mix_render(AudioMix *m,int16_t *out,size_t frames) {
    if(!m || !m->rate || !out || frames>SIZE_MAX/(2*sizeof *out))return;
    while(frames) {
        size_t count=frames>AUDIO_MIX_BLOCK?AUDIO_MIX_BLOCK:frames;
        if(m->paused){memset(out,0,count*2*sizeof *out);out+=count*2;frames-=count;continue;}
        int32_t sum[AUDIO_MIX_BLOCK*2]={0};
        for(unsigned i=0;i<AUDIO_MIX_VOICES;++i) {
            AudioMixVoice *v=&m->voices[i];if(!v->id)continue;
            uint64_t end=(uint64_t)v->source.frames<<32;
            for(size_t n=0;n<count && v->id;++n) {
                uint32_t frame=(uint32_t)(v->phase>>32),fraction=(uint32_t)v->phase;
                uint32_t next=frame+1;
                if(next==v->source.frames)next=v->loop?0:frame;
                int l,r,l2=0,r2=0;
                int ok=sample(v,frame,&l,&r);
                if(ok && fraction){
                    /* The first frame was sampled at voice start. Look ahead
                     * across a loop using that copy; seek only on actual wrap.
                     * Otherwise slow pitch repeatedly seeks tail/head/tail. */
                    if(!next && v->loop){l2=v->first_l;r2=v->first_r;}
                    else ok=sample(v,next,&l2,&r2);
                }
                if(!ok) {
                    ++m->source_errors;release(v);break;
                }
                if(fraction) {
                    l+=(int)(((int64_t)l2-l)*fraction/4294967296LL);
                    r+=(int)(((int64_t)r2-r)*fraction/4294967296LL);
                }
                sum[n*2]+=(int32_t)((int64_t)l*v->gain_l/32768);
                sum[n*2+1]+=(int32_t)((int64_t)r*v->gain_r/32768);
                v->phase+=v->step;
                if(v->phase>=end) {
                    if(v->loop)v->phase%=end;
                    else release(v);
                }
            }
        }
        for(size_t n=0;n<count*2;++n) {
            int32_t value=sum[n];
            if(value>32767){value=32767;++m->clipped_samples;}
            else if(value< -32768){value= -32768;++m->clipped_samples;}
            out[n]=(int16_t)value;
        }
        out+=count*2;frames-=count;
    }
}
