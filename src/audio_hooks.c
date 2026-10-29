#include "audio_hooks.h"
#include "compat_libc.h"
#include "android_log.h"
#include <stdatomic.h>
#include <string.h>
static AudioRuntime *runtime;
static AudioOutput *output;
static atomic_int enabled;
static unsigned polls;
static uint32_t token(void *p){return (uint32_t)(uintptr_t)p;}
static void *handle(uint32_t id){return (void*)(uintptr_t)id;}
static void *new_music(const char *path){return atomic_load(&enabled)?handle(audio_runtime_asset(runtime,path,1)):NULL;}
static void *new_sound(const char *path){return atomic_load(&enabled)?handle(audio_runtime_asset(runtime,path,0)):NULL;}
static void play_music(void *id){if(atomic_load(&enabled))audio_runtime_music_play(runtime,token(id));}
static void dispose_music(void *id){if(atomic_load(&enabled))audio_runtime_dispose_music(runtime,token(id));}
static void stop(void *id){if(atomic_load(&enabled))audio_runtime_stop(runtime,token(id));}
static int playing(void *id){return atomic_load(&enabled)?audio_runtime_playing(runtime,token(id)):0;}
static void loop(void *id,int value){if(atomic_load(&enabled))audio_runtime_loop(runtime,token(id),value);}
static void ANDROID_ABI volume(void *id,float value){if(atomic_load(&enabled))audio_runtime_volume(runtime,token(id),value);}
static float ANDROID_ABI get_volume(void *id){return atomic_load(&enabled)?audio_runtime_get_volume(runtime,token(id)):0;}
static int ANDROID_ABI play2(void *asset,float vol,float pitch,float pan){
 uint32_t id=atomic_load(&enabled)?audio_runtime_sound_play(runtime,token(asset),vol,pitch,pan,0):0;
 return id?(int)id:-1;
}
static int ANDROID_ABI play(void *asset,float vol){return play2(asset,vol,1,0);}
static int ANDROID_ABI sound_loop(void *asset,float vol){
 uint32_t id=atomic_load(&enabled)?audio_runtime_sound_play(runtime,token(asset),vol,1,0,1):0;
 return id?(int)id:-1;
}
static void pause_all(void){if(atomic_load(&enabled))audio_runtime_pause(runtime,1);}
static void resume_all(void){if(atomic_load(&enabled))audio_runtime_pause(runtime,0);}
void audio_hooks_disable(void){atomic_store(&enabled,0);}
void audio_hooks_poll(void){
 if(!atomic_load(&enabled)||++polls%120)return;
 AudioOutputStats o=audio_output_stats(output);AudioRuntimeStats r=audio_runtime_stats(runtime);
 android_log_print(4,"NativeAudio","blocks=%llu elapsed_us=%llu render_us=%llu max_render_us=%llu slow=%llu active=%u prepared=%llu prepare_us=%llu max_prepare_us=%llu ready=%u loader_busy=%u starts=%llu failed=%llu rejected=%llu source_errors=%llu clipped=%llu underruns=%llu queue_wait_us=%llu max_queue_wait_us=%llu queued=%u produced=%llu output_error=%d running=%d",
  (unsigned long long)o.blocks,(unsigned long long)o.elapsed_us,(unsigned long long)o.render_us,(unsigned long long)o.max_render_us,(unsigned long long)o.slow_blocks,r.active,
  (unsigned long long)r.prepared,(unsigned long long)r.prepare_us,(unsigned long long)r.max_prepare_us,r.ready,r.loader_busy,
  (unsigned long long)r.starts,(unsigned long long)r.failed_starts,(unsigned long long)r.rejected_starts,
  (unsigned long long)r.source_errors,(unsigned long long)r.clipped_samples,
  (unsigned long long)o.underruns,(unsigned long long)o.queue_wait_us,(unsigned long long)o.max_queue_wait_us,o.queued,
  (unsigned long long)o.produced,o.error,o.running);
 if(o.error||!o.running){audio_hooks_disable();android_log_print(6,"NativeAudio","Output stopped; playback requests disabled until next launch");}
}
int audio_hooks_configure(ElfImage *im,AudioRuntime *r,AudioOutput *o){
 const char *names[]={"nativePs4_newMusic","nativePs4_playMusic","nativePs4_disposeMusic","nativePs4_setMusicVol","nativePs4_getMusicVol","nativePs4_stopSound","nativePs4_setLoop","nativePs4_isPlaying","nativePs4_newSound","nativePs4_playSound","nativePs4_playSound2","nativePs4_soundLoop","nativePs4_SetSoundVol","PauseAllSounds","ResumeAllSounds"};
 uintptr_t targets[]={(uintptr_t)new_music,(uintptr_t)play_music,(uintptr_t)dispose_music,(uintptr_t)volume,(uintptr_t)get_volume,(uintptr_t)stop,(uintptr_t)loop,(uintptr_t)playing,(uintptr_t)new_sound,(uintptr_t)play,(uintptr_t)play2,(uintptr_t)sound_loop,(uintptr_t)volume,(uintptr_t)pause_all,(uintptr_t)resume_all};
 uint32_t slots[15];
 if(!im||!im->relocated||!r||!o||runtime)return -1;
 for(unsigned i=0;i<15;++i){
  uint32_t value,size,current;
  if(elf_image_symbol(im,names[i],&value,&size)||elf_image_relocation_target(im,names[i],22,&slots[i])||
   (slots[i]&3)||!elf_image_contains(im,slots[i],4,2)||!elf_image_contains(im,value&~1u,2,5))return -1;
  memcpy(&current,im->base+slots[i],4);if(current!=(uint32_t)(uintptr_t)(im->base+value))return -1;
  for(unsigned j=0;j<i;++j)if(slots[j]==slots[i])return -1;
 }
 runtime=r;output=o;
 for(unsigned i=0;i<15;++i){uint32_t target=(uint32_t)targets[i];memcpy(im->base+slots[i],&target,4);}
 atomic_store(&enabled,1);return 0;
}
