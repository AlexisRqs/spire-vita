#include "audio_vorbis.h"
#include <limits.h>
#include <string.h>
#define STB_VORBIS_MAX_CHANNELS 2
#define STB_VORBIS_NO_PUSHDATA_API
#include "../vendor/stb/stb_vorbis.c"

int audio_vorbis_open(AudioVorbis *s,FILE *file,void *arena,size_t bytes) {
    if(!s)return 0;
    memset(s,0,sizeof *s);
    if(!file || !arena || ((uintptr_t)arena&7) || bytes>INT_MAX || bytes<4096)return 0;
    stb_vorbis_alloc alloc={(char*)arena,(int)bytes};
    if(ftell(file)!=0 || fseek(file,0,SEEK_END))return 0;
    long length=ftell(file);
    if(length<=0 || (uint64_t)length>UINT32_MAX || fseek(file,0,SEEK_SET))return 0;
    stb_vorbis *decoder=stb_vorbis_open_file_section(file,0,&s->error,&alloc,(unsigned)length);
    if(!decoder)return 0;
    stb_vorbis_info info=stb_vorbis_get_info(decoder);
    uint32_t frames=stb_vorbis_stream_length_in_samples(decoder);
    if(!frames || frames>INT32_MAX || info.sample_rate<8000 || info.sample_rate>192000 ||
       (info.channels!=1 && info.channels!=2) || !stb_vorbis_seek_start(decoder)) {
        s->error=stb_vorbis_get_error(decoder);stb_vorbis_close(decoder);return 0;
    }
    s->decoder=decoder;s->file=file;s->frames=frames;s->rate=info.sample_rate;
    s->channels=(unsigned)info.channels;s->setup_bytes=info.setup_memory_required;
    s->setup_temp_bytes=info.setup_temp_memory_required;s->decode_temp_bytes=info.temp_memory_required;
    return 1;
}
size_t audio_vorbis_read(AudioVorbis *s,int16_t *out,size_t frames) {
    if(!s || !s->decoder || !out || s->failed || frames>INT_MAX/s->channels)return 0;
    if(frames>s->frames-s->position)frames=s->frames-s->position;
    if(!frames)return 0;
    int got=stb_vorbis_get_samples_short_interleaved(s->decoder,(int)s->channels,out,(int)(frames*s->channels));
    s->error=stb_vorbis_get_error(s->decoder);
    if(got<0 || (size_t)got!=frames || s->error || ferror(s->file))s->failed=1;
    if(got<0)return 0;
    s->position+=(unsigned)got;return (size_t)got;
}
int audio_vorbis_seek(AudioVorbis *s,uint32_t frame) {
    if(!s || !s->decoder || s->failed || frame>s->frames)return 0;
    if(frame==s->frames){s->position=frame;return 1;}
    if(!stb_vorbis_seek(s->decoder,frame)){s->failed=1;s->error=stb_vorbis_get_error(s->decoder);return 0;}
    s->position=frame;return 1;
}
void audio_vorbis_close(AudioVorbis *s) {
    if(!s || !s->decoder)return;
    stb_vorbis_close(s->decoder);fclose(s->file);s->decoder=NULL;s->file=NULL;
}
