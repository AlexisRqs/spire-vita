#include "audio_wave.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { uint8_t bytes[96]; size_t size, calls, max_read; int fail_data; } Fixture;
static void put16(uint8_t *p,unsigned v) { p[0]=v; p[1]=v>>8; }
static void put32(uint8_t *p,uint32_t v) { put16(p,v);put16(p+2,v>>16); }
static size_t reader(void *ctx,uint64_t pos,void *out,size_t n) {
    Fixture *f=ctx; ++f->calls;
    if(n>f->max_read)f->max_read=n;
    if(pos>f->size || n>f->size-pos || (f->fail_data && pos>=60))return 0;
    memcpy(out,f->bytes+(size_t)pos,n);return n;
}
static Fixture fixture(void) {
    Fixture f={.size=92}; uint8_t *p=f.bytes;
    memcpy(p,"RIFF",4);put32(p+4,84);memcpy(p+8,"WAVEfmt ",8);put32(p+16,20);
    put16(p+20,17);put16(p+22,2);put32(p+24,44100);put32(p+28,16000);
    put16(p+32,16);put16(p+34,4);put16(p+36,2);put16(p+38,9);
    memcpy(p+40,"fact",4);put32(p+44,4);put32(p+48,13);
    memcpy(p+52,"data",4);put32(p+56,32);
    put16(p+60,100);put16(p+64,200);put16(p+76,300);put16(p+80,400);
    return f;
}
int main(void) {
    Fixture f=fixture(); AudioWave s; int16_t out[40];
    assert(audio_wave_open(&s,reader,&f,f.size));
    assert(s.channels==2 && s.rate==44100 && s.frames==13);
    assert(audio_wave_read(&s,out,4)==4 && out[0]==100 && out[1]==200);
    size_t calls=f.calls;
    assert(audio_wave_read(&s,out,5)==5 && f.calls==calls); /* Same block cached. */
    assert(audio_wave_read(&s,out,20)==4 && out[0]==300 && out[1]==400);
    assert(audio_wave_read(&s,out,20)==0 && !s.failed);
    assert(audio_wave_seek(&s,8));
    assert(audio_wave_read(&s,out,3)==3);
    assert(out[0]==100 && out[1]==200 && out[2]==300 && out[3]==400);
    assert(audio_wave_seek(&s,13));assert(!audio_wave_seek(&s,14));
    for(unsigned i=0;i<1000;++i) {
        assert(audio_wave_seek(&s,0));
        assert(audio_wave_read(&s,out,20)==13);
        assert(out[0]==100 && out[24]==300);
    }
    assert(f.max_read<=20);
    f=fixture();assert(audio_wave_open(&s,reader,&f,f.size));f.fail_data=1;
    assert(audio_wave_read(&s,out,20)==0 && s.failed);
    assert(!audio_wave_seek(&s,0));
    for(size_t n=0;n<92;++n) {
        f=fixture();assert(!audio_wave_open(&s,reader,&f,n));assert(!s.read_at);
    }
    const unsigned offsets[]={0,8,20,22,24,32,34,36,38,48,56};
    for(unsigned i=0;i<sizeof offsets/sizeof *offsets;++i) {
        f=fixture();memset(f.bytes+offsets[i],0xff,4);
        /* Rate is unrestricted nonzero; reject zero separately. */
        if(offsets[i]==24)memset(f.bytes+24,0,4);
        assert(!audio_wave_open(&s,reader,&f,f.size));assert(!s.read_at);
    }
    f=fixture();put32(f.bytes+48,0);assert(audio_wave_open(&s,reader,&f,f.size));
    assert(audio_wave_read(&s,out,20)==0 && !s.failed);
    f=fixture();f.bytes[62]=89;assert(audio_wave_open(&s,reader,&f,f.size));
    assert(audio_wave_read(&s,out,20)==0 && s.failed);
    /* Deterministic malformed-header corpus with strict bounded reader. */
    uint32_t rng=1;
    for(unsigned i=0;i<10000;++i) {
        f=fixture();rng=rng*1664525u+1013904223u;unsigned pos=rng%92;
        rng=rng*1664525u+1013904223u;f.bytes[pos]=(uint8_t)(rng>>24);
        if(audio_wave_open(&s,reader,&f,f.size)) {
            size_t n=audio_wave_read(&s,out,20);assert(n<=18);
        }
    }
    printf("wave reader bounds, EOF, seek, 1000 loops, I/O failure, malformed headers: pass; state %zu bytes\n",sizeof s);
}
