#include "android_frame_loop.h"
#include <pthread.h>
#include <string.h>
static AndroidFramePump frame_pump;
static void *frame_context;
static pthread_t frame_owner;
/* Configure before starting the engine/other threads; never mutate concurrently. */
int android_frame_loop_configure(AndroidFramePump pump,void *context){
 frame_owner=pthread_self();frame_context=context;frame_pump=pump;return 0;
}
int android_frame_poll(int timeout,int *fd,int *events,void **data){
 if(fd)*fd=0;
 if(events)*events=0;
 if(data)*data=NULL;
 if(!frame_pump||!pthread_equal(pthread_self(),frame_owner))return -4;
 int result=frame_pump(timeout,frame_context);
 return result==-1||result==-3?result:-4;
}
uintptr_t android_frame_loop_lookup(const char *name){
 return name&&!strcmp(name,"ALooper_pollOnce")?(uintptr_t)android_frame_poll:0;
}
