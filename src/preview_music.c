#include "preview_music.h"
#include <pthread.h>
#include <string.h>
/* Silent preview only. Opaque generation-tagged IDs are never dereferenced by
 * original music code. No assets, decoders, stream threads or heap allocations.
 * Non-looping duration is not simulated: playing lasts until stop/dispose. */
#define COUNT 64
#define TAG 0xd5000000u
static struct {uint32_t generation;float volume;int active,playing,loop;} records[COUNT];
static uint32_t serial;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static void (*sound_stop)(void *);
static void (*sound_loop)(void *,int);
static int (*other_playing)(void *);
static int tagged(void *h){return ((uintptr_t)h&~(uintptr_t)0xffffffu)==TAG;}
static int index_of(void *h){
 uint32_t token=(uint32_t)(uintptr_t)h;int i=token&63;
 return tagged(h)&&records[i].active&&records[i].generation==((token>>6)&0x3ffff)?i:-1;
}
void preview_music_bind(void (*stop)(void *),void (*loop)(void *,int),int (*playing)(void *)){
 sound_stop=stop;sound_loop=loop;other_playing=playing;
}
void *preview_music_new(const char *path){
 (void)path;void *result=NULL;pthread_mutex_lock(&lock);
 for(int i=0;i<COUNT&&serial<0x3ffff;i++)if(!records[i].active){
  records[i].generation=++serial;records[i].volume=1;records[i].active=1;records[i].playing=records[i].loop=0;
  result=(void*)(uintptr_t)(TAG|(serial<<6)|(unsigned)i);break;
 }
 pthread_mutex_unlock(&lock);return result;
}
void preview_music_play(void *h){pthread_mutex_lock(&lock);int i=index_of(h);if(i>=0)records[i].playing=1;pthread_mutex_unlock(&lock);}
void preview_music_dispose(void *h){pthread_mutex_lock(&lock);int i=index_of(h);if(i>=0)records[i].active=records[i].playing=0;pthread_mutex_unlock(&lock);}
void preview_music_stop(void *h){
 if(!h||(uintptr_t)h==0xffffffffu)return;
 if(!tagged(h)){if(sound_stop)sound_stop(h);return;}
 pthread_mutex_lock(&lock);int i=index_of(h);if(i>=0)records[i].playing=0;pthread_mutex_unlock(&lock);
}
int preview_music_playing(void *h){
 if(!h||(uintptr_t)h==0xffffffffu)return 0;
 if(!tagged(h))return other_playing?other_playing(h):0;
 pthread_mutex_lock(&lock);int i=index_of(h),v=i>=0?records[i].playing:0;pthread_mutex_unlock(&lock);return v;
}
void preview_music_loop(void *h,int loop){
 if(!h||(uintptr_t)h==0xffffffffu)return;
 if(!tagged(h)){if(sound_loop)sound_loop(h,loop);return;}
 pthread_mutex_lock(&lock);int i=index_of(h);if(i>=0)records[i].loop=!!loop;pthread_mutex_unlock(&lock);
}
void ANDROID_ABI preview_music_set_volume(void *h,float v){pthread_mutex_lock(&lock);int i=index_of(h);if(i>=0)records[i].volume=v;pthread_mutex_unlock(&lock);}
float ANDROID_ABI preview_music_get_volume(void *h){pthread_mutex_lock(&lock);int i=index_of(h);float v=i>=0?records[i].volume:0;pthread_mutex_unlock(&lock);return v;}
/* Sound assets share an inert token; no native consumer may dereference it.
 * Playback returns the engine's unavailable-source sentinel (-1). */
static unsigned char silent_sound_asset;
void *preview_sound_new(const char *path){(void)path;return &silent_sound_asset;}
int ANDROID_ABI preview_sound_play(void *asset,float volume){(void)asset;(void)volume;return -1;}
int ANDROID_ABI preview_sound_play2(void *asset,float volume,float pitch,float pan){(void)pitch;(void)pan;return preview_sound_play(asset,volume);}
void ANDROID_ABI preview_sound_volume(void *source,float volume){(void)source;(void)volume;}
int preview_music_configure(ElfImage *im){
 const char *names[]={"nativePs4_newMusic","nativePs4_playMusic","nativePs4_disposeMusic","nativePs4_setMusicVol","nativePs4_getMusicVol","nativePs4_stopSound","nativePs4_setLoop","nativePs4_isPlaying","nativePs4_newSound","nativePs4_playSound","nativePs4_playSound2","nativePs4_soundLoop","nativePs4_SetSoundVol"};
 uintptr_t targets[]={(uintptr_t)preview_music_new,(uintptr_t)preview_music_play,(uintptr_t)preview_music_dispose,(uintptr_t)preview_music_set_volume,(uintptr_t)preview_music_get_volume,(uintptr_t)preview_music_stop,(uintptr_t)preview_music_loop,(uintptr_t)preview_music_playing,(uintptr_t)preview_sound_new,(uintptr_t)preview_sound_play,(uintptr_t)preview_sound_play2,(uintptr_t)preview_sound_play,(uintptr_t)preview_sound_volume};
 uint32_t slots[13],values[13];
 if(!im||!im->relocated)return -1;
 for(unsigned i=0;i<13;i++){
  uint32_t size,current;
  if(elf_image_symbol(im,names[i],&values[i],&size)||elf_image_relocation_target(im,names[i],22,&slots[i])||
   (slots[i]&3)||!elf_image_contains(im,slots[i],4,2)||!elf_image_contains(im,values[i]&~1u,2,5))return -1;
  memcpy(&current,im->base+slots[i],4);if(current!=(uint32_t)(uintptr_t)(im->base+values[i]))return -1;
 }
 preview_music_bind((void (*)(void*))(im->base+values[5]),(void (*)(void*,int))(im->base+values[6]),(int (*)(void*))(im->base+values[7]));
 for(unsigned i=0;i<13;i++){uint32_t target=targets[i];memcpy(im->base+slots[i],&target,4);}return 0;
}
