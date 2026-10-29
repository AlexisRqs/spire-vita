#include "android_input.h"
#include <math.h>
#include <string.h>
int android_motion_init(AndroidMotion *e,int action,float x,float y,float scroll,int mouse){
 if(!e||!isfinite(x)||!isfinite(y)||!isfinite(scroll)||((action<0||action>3)&&action!=8)||(action==8&&!mouse))return -1;
 *e=(AndroidMotion){2,mouse?0x2002:0x1002,action,x,y,scroll};return 0;
}
int32_t android_input_type(const AndroidMotion *e){return e?e->type:0;}
int32_t android_input_source(const AndroidMotion *e){return e?e->source:0;}
int32_t android_motion_action(const AndroidMotion *e){return e?e->action:3;}
ANDROID_ABI float android_motion_x(const AndroidMotion *e,size_t i){return e&&!i?e->x:0;}
ANDROID_ABI float android_motion_y(const AndroidMotion *e,size_t i){return e&&!i?e->y:0;}
ANDROID_ABI float android_motion_axis(const AndroidMotion *e,int32_t axis,size_t i){return e&&!i&&axis==9?e->scroll:0;}
uintptr_t android_input_lookup(const char *name){
 if(!name)return 0;
#define INPUT(n,f) if(!strcmp(name,n))return (uintptr_t)f;
 INPUT("AInputEvent_getType",android_input_type)
 INPUT("AInputEvent_getSource",android_input_source)
 INPUT("AMotionEvent_getAction",android_motion_action)
 /* The APK motion handler also calls the key-action accessor. Our owned
  * event has one shared action field; this does not reinterpret OS events. */
 INPUT("AKeyEvent_getAction",android_motion_action)
 INPUT("AMotionEvent_getX",android_motion_x)
 INPUT("AMotionEvent_getY",android_motion_y)
 INPUT("AMotionEvent_getAxisValue",android_motion_axis)
#undef INPUT
 return 0;
}
