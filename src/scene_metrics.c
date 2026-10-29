#include "scene_metrics.h"
#include "android_log.h"
#include <pthread.h>
#include <string.h>
#ifdef __vita__
#include <psp2/kernel/processmgr.h>
#else
#include <time.h>
#endif
uint64_t scene_clock(void){
#ifdef __vita__
 return sceKernelGetProcessTimeWide();
#else
 struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000+t.tv_nsec/1000;
#endif
}
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static struct {const char *name;unsigned token;uint64_t start,end,io,bytes;} pending[16];
static uint64_t io_us,io_bytes,last_present;
static unsigned serial,frame,frames;
static uint64_t frame_total,frame_max;static unsigned slow50,slow100;
unsigned scene_begin(const char *name){
 uint64_t now=scene_clock();pthread_mutex_lock(&lock);unsigned token=++serial;
 unsigned i;for(i=0;i<16;i++)if(!pending[i].token)break;
 if(i==16){pthread_mutex_unlock(&lock);android_log_print(5,"SceneProfile","pending capacity exceeded: %s",name);return 0;}
 pending[i].name=name;pending[i].token=token;pending[i].start=now;pending[i].end=0;pending[i].io=io_us;pending[i].bytes=io_bytes;
 pthread_mutex_unlock(&lock);android_log_print(4,"SceneProfile","begin id=%u name=%s time_us=%llu",token,name,(unsigned long long)now);return token;
}
void scene_return(unsigned token){
 if(!token)return;
 pthread_mutex_lock(&lock);
 for(unsigned i=0;i<16;i++)if(pending[i].token==token){pending[i].end=scene_clock();break;}
 pthread_mutex_unlock(&lock);
}
void scene_asset_io(uint64_t us,unsigned bytes){pthread_mutex_lock(&lock);io_us+=us;io_bytes+=bytes;pthread_mutex_unlock(&lock);}
void scene_present(void){
 uint64_t now=scene_clock();pthread_mutex_lock(&lock);frame++;
 for(unsigned i=0;i<16;i++)if(pending[i].token&&pending[i].end){
  android_log_print(4,"SceneProfile","end id=%u name=%s work_us=%llu next_swap_us=%llu asset_read_us=%llu asset_bytes=%llu frame=%u",pending[i].token,pending[i].name,(unsigned long long)(pending[i].end-pending[i].start),(unsigned long long)(now-pending[i].start),(unsigned long long)(io_us-pending[i].io),(unsigned long long)(io_bytes-pending[i].bytes),frame);
  pending[i].token=0;
 }
 if(last_present){uint64_t d=now-last_present;frame_total+=d;if(d>frame_max)frame_max=d;slow50+=d>50000;slow100+=d>100000;frames++;}
 last_present=now;
 if(frames==120){android_log_print(4,"FrameProfile","frame=%u samples=%u mean_us=%llu max_us=%llu over50ms=%u over100ms=%u",frame,frames,(unsigned long long)(frame_total/frames),(unsigned long long)frame_max,slow50,slow100);frames=0;frame_total=frame_max=0;slow50=slow100=0;}
 pthread_mutex_unlock(&lock);
}
