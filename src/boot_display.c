#include "loading_screen.h"
#include "launcher_background.h"
#include "android_log.h"
#include "debugScreen.h"
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
static SceUID block=-1;
static uint32_t *buffers;
static pthread_t worker;
static atomic_int running;
static int started;
static pthread_mutex_t phase_lock=PTHREAD_MUTEX_INITIALIZER;
static char phase[64]="Preparing engine";
static const unsigned char *font;
static void *animate(void *unused){
 (void)unused;uint64_t start=sceKernelGetProcessTimeWide();unsigned frame=0;
 while(atomic_load(&running)){
  char label[64];pthread_mutex_lock(&phase_lock);memcpy(label,phase,sizeof(label));pthread_mutex_unlock(&phase_lock);
  uint32_t *p=buffers+(frame++%2)*(2*1024*1024/4);
  if(launcher_background_active()){launcher_background_tick();launcher_background_draw(p,font);}
  else loading_screen_draw(p,font,label,(unsigned)((sceKernelGetProcessTimeWide()-start)/1000));
  SceDisplayFrameBuf fb={sizeof(fb),p,960,SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,960,544};
  sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME);sceKernelDelayThread(16667);
 }
 return NULL;
}
int boot_display_start(void){
 if(started||block>=0)return -1;
 PsvDebugScreenFont *f=psvDebugScreenGetFont();if(!f||f->width!=8||f->height!=8||f->first!=0)return -1;
 font=f->glyphs;
 block=sceKernelAllocMemBlock("SpireLoading",SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,4*1024*1024,NULL);
 if(block<0)return -1;
 if(sceKernelGetMemBlockBase(block,(void**)&buffers)<0){sceKernelFreeMemBlock(block);block=-1;return -1;}
 atomic_store(&running,1);
 if(pthread_create(&worker,NULL,animate,NULL)){atomic_store(&running,0);sceKernelFreeMemBlock(block);block=-1;return -1;}
 started=1;return 0;
}
void boot_display_message(const char *message){
 const char *next=NULL;unsigned stage=0;
 if(strstr(message,"Loc Strings load time:")){next="Loading images";stage=2;};
 if(strstr(message,"Card Image load time:")){next="Loading scene textures";stage=2;};
 if(strstr(message,"Texture load time:")){next="Loading fonts";stage=3;};
 if(strstr(message,"Font load time:")){next="Loading cards";stage=4;};
 if(strstr(message,"Relic load time:")){next="Loading characters";stage=6;};
 if(next)launcher_background_stage(stage,next);
 if(next){pthread_mutex_lock(&phase_lock);strcpy(phase,next);pthread_mutex_unlock(&phase_lock);}
}
void boot_display_stop(void){if(started){atomic_store(&running,0);pthread_join(worker,NULL);started=0;}}
void boot_display_release(void){
 if(started||block<0)return;
 /* VitaGL's display queue is asynchronous. Do not free a current OR queued
  * loading framebuffer; retry on the next swap without blocking the game. */
 for(int sync=0;sync<=1;sync++){
  SceDisplayFrameBuf fb={.size=sizeof(fb)};
  if(sceDisplayGetFrameBuf(&fb,sync)<0)return;
  uintptr_t p=(uintptr_t)fb.base,lo=(uintptr_t)buffers;
  if(p>=lo&&p<lo+4*1024*1024)return;
 }
 sceKernelFreeMemBlock(block);block=-1;buffers=NULL;
 android_log_print(4,"Loading","Framebuffer handoff complete; released 4 MiB");
}
