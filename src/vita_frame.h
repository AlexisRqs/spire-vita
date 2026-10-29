#ifndef VITA_FRAME_H
#define VITA_FRAME_H
#include "android_platform.h"
#include "android_input.h"
typedef int (*EngineInput)(void *app,const AndroidMotion *event);
typedef void (*EngineBack)(void);
int vita_frame_configure(AndroidApp32 *app,EngineInput input,EngineBack back);
int vita_frame_pointer(float *x,float *y);
void vita_frame_shutdown(void);
#endif
