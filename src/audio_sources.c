#include "audio_sources.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static size_t file_read(void *ctx,uint64_t offset,void *out,size_t bytes) {
    AudioFileSource *s=ctx;
    if(offset>LONG_MAX || fseek(s->file,(long)offset,SEEK_SET))return 0;
    return fread(out,1,bytes,s->file);
}
static size_t read_pcm(void *ctx,int16_t *out,size_t frames) {
    AudioFileSource *s=ctx;
    return s->music?audio_vorbis_read(&s->decoder.vorbis,out,frames):audio_wave_read(&s->decoder.wave,out,frames);
}
static int seek_pcm(void *ctx,uint32_t frame) {
    AudioFileSource *s=ctx;
    return s->music?audio_vorbis_seek(&s->decoder.vorbis,frame):audio_wave_seek(&s->decoder.wave,frame);
}
static void close_pcm(void *ctx) {
    AudioFileSource *s=ctx;
    if(!s->active)return;
    if(s->music)audio_vorbis_close(&s->decoder.vorbis);
    else if(s->file)fclose(s->file);
    free(s->arena);s->arena=NULL;s->file=NULL;s->active=0;
}
static int valid_path(const char *path,int music) {
    if(!path)return 0;
    const char *prefix=music?"audio/music/":"audio/sound/";
    size_t length=strlen(path);
    if(length>=256 || strncmp(path,prefix,strlen(prefix)))return 0;
    const char *component=path;
    for(const char *p=path;;++p) {
        if(*p==':' || *p=='\\' || (*p && (unsigned char)*p<32))return 0;
        if(*p=='/' || !*p) {
            size_t n=(size_t)(p-component);
            if(!n || (n==1 && component[0]=='.') || (n==2 && component[0]=='.' && component[1]=='.'))return 0;
            if(!*p)return 1;
            component=p+1;
        }
    }
}
int audio_sources_init(AudioSources *p,const char *root) {
    if(!p || !root || !*root || strlen(root)>=sizeof p->root)return 0;
    memset(p,0,sizeof *p);strcpy(p->root,root);return 1;
}
unsigned audio_sources_active(const AudioSources *p) {
    unsigned n=0;if(p)for(unsigned i=0;i<AUDIO_SOURCE_MUSIC+AUDIO_SOURCE_EFFECTS;++i)n+=!!p->sources[i].active;
    return n;
}
int audio_sources_open(AudioSources *p,const char *path,int music,AudioMixSource *out) {
    if(!out)return 0;
    memset(out,0,sizeof *out);
    if(!p || !p->root[0])return 0;
    music=!!music;
    if(!valid_path(path,music)){++p->open_errors;return 0;}
    unsigned start=music?0:AUDIO_SOURCE_MUSIC,end=music?AUDIO_SOURCE_MUSIC:AUDIO_SOURCE_MUSIC+AUDIO_SOURCE_EFFECTS;
    AudioFileSource *s=NULL;
    for(unsigned i=start;i<end;++i)if(!p->sources[i].active){s=&p->sources[i];break;}
    if(!s){++p->pool_full;return 0;}
    char full[1280];int n=snprintf(full,sizeof full,"%s/%s",p->root,path);
    if(n<0 || (size_t)n>=sizeof full){++p->open_errors;return 0;}
    memset(s,0,sizeof *s);s->music=music;
    s->file=fopen(full,"rb");if(!s->file){++p->open_errors;return 0;}
    if(setvbuf(s->file,s->io_buffer,_IOFBF,sizeof s->io_buffer))goto fail;
    uint32_t frames,rate;unsigned channels;
    if(music) {
        s->arena=malloc(AUDIO_SOURCE_ARENA);
        if(!s->arena || !audio_vorbis_open(&s->decoder.vorbis,s->file,s->arena,AUDIO_SOURCE_ARENA))goto fail;
        frames=s->decoder.vorbis.frames;rate=s->decoder.vorbis.rate;channels=s->decoder.vorbis.channels;
    } else {
        if(fseek(s->file,0,SEEK_END))goto fail;
        long size=ftell(s->file);
        if(size<0 || !audio_wave_open(&s->decoder.wave,file_read,s,(uint64_t)size))goto fail;
        frames=s->decoder.wave.frames;rate=s->decoder.wave.rate;channels=s->decoder.wave.channels;
        /* Prime the first IMA block on the preparer. Rewinding within that
         * decoded block keeps it cached for the first audio-worker request. */
        int16_t first[2];
        if(audio_wave_read(&s->decoder.wave,first,1)!=1 || !audio_wave_seek(&s->decoder.wave,0))goto fail;
    }
    s->active=1;
    *out=(AudioMixSource){s,read_pcm,seek_pcm,close_pcm,frames,rate,channels};return 1;
fail:
    fclose(s->file);free(s->arena);s->file=NULL;s->arena=NULL;++p->open_errors;return 0;
}
