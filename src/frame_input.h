#ifndef FRAME_INPUT_H
#define FRAME_INPUT_H
#include "android_input.h"
/* Platform-neutral single-pointer controller. Coordinates are display pixels. */
typedef struct {float x,y;int pressed,escape,scroll_active;} FrameInput;
typedef struct {int touched,click,back;float touch_x,touch_y,dx,dy,scroll;} FrameSample;
typedef void (*FrameMotionSink)(const AndroidMotion *,void *);
typedef void (*FrameBackSink)(void *);
void frame_pointer_delta(unsigned lx,unsigned ly,unsigned rx,unsigned ry,int horizontal,int vertical,float seconds,float *dx,float *dy);
int frame_input_update(FrameInput *state,const FrameSample *sample,int width,int height,
 FrameMotionSink motion,FrameBackSink back,void *context);
#endif
