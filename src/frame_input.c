#include "frame_input.h"
#include <math.h>
static float clamp(float v,float maximum){return v<0?0:v>maximum?maximum:v;}
static float axis(unsigned value){
 int n=(int)(value>255?255:value)-128;
 if(n>-24&&n<24)return 0;
 return n/128.0f;
}
void frame_pointer_delta(unsigned lx,unsigned ly,unsigned rx,unsigned ry,int horizontal,int vertical,float seconds,float *dx,float *dy){
 if(!isfinite(seconds)||seconds<0)seconds=0;
 if(seconds>0.1f)seconds=0.1f;
 float x=axis(lx),y=axis(ly),r=axis(rx),s=axis(ry);
 if(fabsf(r)>fabsf(x))x=r;
 if(fabsf(s)>fabsf(y))y=s;
 if(horizontal)x=horizontal>0?1:-1;
 if(vertical)y=vertical>0?1:-1;
 *dx=x*480*seconds;*dy=y*480*seconds;
}
int frame_input_update(FrameInput *state,const FrameSample *s,int width,int height,
 FrameMotionSink motion,FrameBackSink back,void *context){
 if(!state||!s||!motion||width<=0||height<=0||!isfinite(state->x)||!isfinite(state->y)||
    !isfinite(s->touch_x)||!isfinite(s->touch_y)||!isfinite(s->dx)||!isfinite(s->dy)||!isfinite(s->scroll))return -1;
 float x=clamp(s->touched?s->touch_x:state->x+s->dx,width-1);
 float y=clamp(s->touched?s->touch_y:state->y+s->dy,height-1);
 int pressed=!!(s->touched||s->click);AndroidMotion event;
 /* Send release at the last held position; pointer hover is a separate move. */
 if(state->pressed&&!pressed){
  android_motion_init(&event,1,state->x,state->y,0,0);motion(&event,context);
 }
 if(pressed||x!=state->x||y!=state->y||state->scroll_active){
  android_motion_init(&event,pressed&&!state->pressed?0:2,x,y,0,0);motion(&event,context);
 }
 if(s->scroll){android_motion_init(&event,8,x,y,s->scroll,1);motion(&event,context);}
 if(s->back&&!state->escape&&back)back(context);
 state->x=x;state->y=y;state->pressed=pressed;state->escape=!!s->back;state->scroll_active=s->scroll!=0;return 0;
}
