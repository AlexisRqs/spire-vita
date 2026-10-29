#include "audio_wave.h"
#include <string.h>

static unsigned le16(const uint8_t *p) { return p[0] | ((unsigned)p[1] << 8); }
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
int audio_wave_open(AudioWave *s, AudioReadAt reader, void *context, uint64_t bytes) {
    if (!s) return 0;
    memset(s, 0, sizeof *s);
    if (!reader || bytes < 12) return 0;
    uint8_t h[20];
    if (reader(context,0,h,12)!=12 || memcmp(h,"RIFF",4) || memcmp(h+8,"WAVE",4)) return 0;
    uint64_t end = 8 + (uint64_t)le32(h+4);
    if (end < 12 || end > bytes) return 0;
    unsigned channels=0, align=0, block_frames=0;
    uint32_t rate=0, frames=0, data_bytes=0;
    uint64_t data_offset=0;
    unsigned seen=0;
    for (uint64_t pos=12; pos<end;) {
        if (end-pos < 8 || reader(context,pos,h,8)!=8) return 0;
        uint32_t n=le32(h+4);
        uint64_t body=pos+8, next=body+n+(n&1);
        if (next>end) return 0;
        if (!memcmp(h,"fmt ",4)) {
            if ((seen&1) || n<20 || reader(context,body,h,20)!=20) return 0;
            seen|=1;
            channels=le16(h+2); rate=le32(h+4); align=le16(h+12); block_frames=le16(h+18);
            if (le16(h)!=17 || (channels!=1 && channels!=2) || !rate ||
                le16(h+14)!=4 || le16(h+16)!=2 || align<4*channels || align>1024 ||
                align%(4*channels) || block_frames!=1+(align-4*channels)*2/channels) return 0;
        } else if (!memcmp(h,"fact",4)) {
            if ((seen&2) || n<4 || reader(context,body,h,4)!=4) return 0;
            seen|=2; frames=le32(h);
        } else if (!memcmp(h,"data",4)) {
            if (seen&4) return 0;
            seen|=4; data_offset=body; data_bytes=n;
        }
        pos=next;
    }
    if (seen!=7 || data_bytes%align || frames>(uint64_t)(data_bytes/align)*block_frames) return 0;
    s->read_at=reader; s->context=context; s->data_offset=data_offset;
    s->data_bytes=data_bytes; s->frames=frames; s->rate=rate;
    s->channels=channels; s->block_bytes=align; s->block_frames=block_frames;
    s->cached_block=UINT64_MAX;
    return 1;
}
int audio_wave_seek(AudioWave *s, uint64_t frame) {
    if (!s || !s->read_at || s->failed || frame>s->frames) return 0;
    s->position=frame;
    return 1;
}
size_t audio_wave_read(AudioWave *s, int16_t *out, size_t requested) {
    if (!s || !s->read_at || !out || s->failed || requested>SIZE_MAX/(2*s->channels)) return 0;
    size_t done=0;
    while (done<requested && s->position<s->frames) {
        uint64_t block=s->position/s->block_frames;
        unsigned within=(unsigned)(s->position%s->block_frames);
        if (block!=s->cached_block) {
            uint64_t offset=s->data_offset+block*s->block_bytes;
            if (s->read_at(s->context,offset,s->compressed,s->block_bytes)!=s->block_bytes) {
                s->failed=1; break;
            }
            size_t decoded=audio_ima_decode_block(s->compressed,s->block_bytes,s->channels,s->pcm,2048);
            if (decoded!=s->block_frames) { s->failed=1; break; }
            s->cached_block=block; s->cached_frames=(unsigned)decoded;
        }
        size_t n=s->cached_frames-within;
        if (n>requested-done) n=requested-done;
        if (n>s->frames-s->position) n=(size_t)(s->frames-s->position);
        memcpy(out+done*s->channels,s->pcm+within*s->channels,n*s->channels*sizeof *out);
        done+=n; s->position+=n;
    }
    return done;
}
