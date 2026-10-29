#include "audio_mix.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {int16_t pcm[32768];unsigned channels,frames,pos,closes,reads,seeks;int fail;} Source;
static size_t read_pcm(void *ctx,int16_t *out,size_t frames) {
    Source *s=ctx;++s->reads;
    if(s->fail)return 0;
    assert(frames<=256 && frames<=s->frames-s->pos);
    memcpy(out,s->pcm+s->pos*s->channels,frames*s->channels*2);s->pos+=(unsigned)frames;return frames;
}
static int seek_pcm(void *ctx,uint32_t frame) {
    Source *s=ctx;++s->seeks;if(s->fail || frame>s->frames)return 0;s->pos=frame;return 1;
}
static void close_pcm(void *ctx){++((Source*)ctx)->closes;}
static AudioMixSource source(Source *s,unsigned frames,unsigned channels,unsigned rate) {
    memset(s,0,sizeof *s);s->frames=frames;s->channels=channels;
    for(unsigned i=0;i<frames;++i){s->pcm[i*channels]=(int16_t)(i*20);if(channels==2)s->pcm[i*channels+1]=-(int16_t)(i*20);}
    return (AudioMixSource){s,read_pcm,seek_pcm,close_pcm,frames,rate,channels};
}
int main(void) {
    AudioMix m;Source s;int16_t out[4096];assert(audio_mix_init(&m,44100));
    AudioMixSource src=source(&s,600,2,44100);
    uint32_t id=audio_mix_start(&m,&src,1,1,0,0);assert(id);
    audio_mix_pause(&m,1);audio_mix_render(&m,out,1000);
    for(unsigned i=0;i<2000;++i)assert(out[i]==0);
    assert(!s.reads && !s.closes && audio_mix_playing(&m,id));
    audio_mix_pause(&m,0);
    audio_mix_render(&m,out,601);
    assert(!memcmp(out,s.pcm,600*4));assert(out[1200]==0 && out[1201]==0);
    assert(!audio_mix_playing(&m,id) && s.closes==1 && s.reads==3 && s.seeks==0);
    audio_mix_stop(&m,id);assert(s.closes==1);
    src=source(&s,4,1,44100);id=audio_mix_start(&m,&src,1,.5f,0,0);assert(id);
    audio_mix_render(&m,out,9);
    const int expected[]={0,10,20,30,40,50,60,60,0};
    for(unsigned i=0;i<9;++i)assert(out[2*i]==expected[i] && out[2*i+1]==expected[i]);
    assert(s.closes==1);
    src=source(&s,6,2,44100);id=audio_mix_start(&m,&src,.5f,2,1,0);assert(id);
    audio_mix_render(&m,out,4);
    assert(out[0]==0 && out[1]==0 && out[2]==0 && out[3]==-20 && out[5]==-40 && out[7]==0);
    src=source(&s,3,1,44100);id=audio_mix_start(&m,&src,1,1,0,1);assert(id);
    audio_mix_render(&m,out,1000);
    for(unsigned i=0;i<1000;++i)assert(out[i*2]==(int)(i%3)*20);
    assert(audio_mix_playing(&m,id) && !s.closes);
    assert(!audio_mix_control(&m,id,NAN,1,0,0));
    assert(!audio_mix_control(&m,id,1,INFINITY,0,0));
    assert(audio_mix_control(&m,id,1,1,0,0));audio_mix_render(&m,out,5);
    assert(out[0]==20 && out[2]==40 && out[4]==0 && s.closes==1);
    /* Stale handle cannot stop the next occupant. */
    uint32_t old=id;src=source(&s,6,1,44100);id=audio_mix_start(&m,&src,1,1,0,0);
    assert(id!=old);audio_mix_stop(&m,old);assert(audio_mix_playing(&m,id));
    s.fail=1;audio_mix_render(&m,out,5);assert(s.closes==1 && m.source_errors==1);
    for(unsigned i=0;i<10;++i)assert(out[i]==0);
    /* 32 overlapping voices: bounded pool, clipping and ownership on failure. */
    Source pool[33];
    for(unsigned i=0;i<33;++i) {
        src=source(&pool[i],1,1,44100);pool[i].pcm[0]=30000;
        uint32_t voice=audio_mix_start(&m,&src,1,1,0,1);assert((voice!=0)==(i<32));
    }
    audio_mix_render(&m,out,1000);for(unsigned i=0;i<2000;++i)assert(out[i]==32767);
    assert(m.clipped_samples==2000 && pool[32].closes==0);
    audio_mix_shutdown(&m);audio_mix_shutdown(&m);
    for(unsigned i=0;i<32;++i)assert(pool[i].closes==1);
    audio_mix_render(&m,out,10);for(unsigned i=0;i<20;++i)assert(out[i]==0);
    /* Stereo 44.1 -> 48 kHz duration; output independent of render chunking. */
    AudioMix other;Source a,b;int16_t one[24000],split[24000];
    assert(audio_mix_init(&m,48000));assert(audio_mix_init(&other,48000));
    src=source(&a,10000,2,44100);assert(audio_mix_start(&m,&src,1,1,0,0));
    src=source(&b,10000,2,44100);assert(audio_mix_start(&other,&src,1,1,0,0));
    audio_mix_render(&m,one,11000);
    for(unsigned i=0;i<11000;++i)audio_mix_render(&other,split+i*2,1);
    assert(!memcmp(one,split,44000));assert(a.closes==1 && b.closes==1);
    /* Q32 rounding may leave one final frame at this rate; bound duration. */
    assert(a.seeks==0 && b.seeks==0);
    for(unsigned i=10886;i<11000;++i)assert(one[i*2]==0 && one[i*2+1]==0);
    assert(one[100*2]>1800 && one[100*2]<1850);
    assert(!audio_mix_start(&m,&src,1,0,0,0));
    m.serial=UINT32_MAX;assert(!audio_mix_start(&m,&src,1,1,0,0));
    printf("mix overlap, gain, pan, pitch, loops, EOF, stale handles, errors, resampling: pass; state %zu bytes\n",sizeof m);
}
