#ifndef ANDROID_INPUT_H
#define ANDROID_INPUT_H
#include "compat_libc.h"
#include <stddef.h>
/* Owned single-pointer events. Never cast an Android OS event to this type. */
typedef struct {int32_t type,source,action;float x,y,scroll;} AndroidMotion;
int android_motion_init(AndroidMotion *event,int action,float x,float y,float scroll,int mouse);
int32_t android_input_type(const AndroidMotion *event);
int32_t android_input_source(const AndroidMotion *event);
int32_t android_motion_action(const AndroidMotion *event);
ANDROID_ABI float android_motion_x(const AndroidMotion *event,size_t pointer);
ANDROID_ABI float android_motion_y(const AndroidMotion *event,size_t pointer);
ANDROID_ABI float android_motion_axis(const AndroidMotion *event,int32_t axis,size_t pointer);
uintptr_t android_input_lookup(const char *name);
#endif
