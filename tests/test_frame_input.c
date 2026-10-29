#include "frame_input.h"
#include <assert.h>
#include <math.h>
static AndroidMotion records[32];static int count,backs;
static void motion(const AndroidMotion *e,void *ctx){assert(ctx==records);records[count++]=*e;}
static void back(void *ctx){assert(ctx==records);backs++;}
int main(void){
 float dx,dy;
 frame_pointer_delta(128,128,128,128,0,0,0.02f,&dx,&dy);assert(dx==0&&dy==0);
 frame_pointer_delta(128,128,255,0,0,0,0.1f,&dx,&dy);assert(dx>47&&dy==-48);
 frame_pointer_delta(255,255,128,128,-1,-1,0.1f,&dx,&dy);assert(dx==-48&&dy==-48);
 frame_pointer_delta(128,128,128,128,1,0,0.08f,&dx,&dy);assert(dx>38&&dx<39&&dy==0);
 frame_pointer_delta(128,128,128,128,1,0,NAN,&dx,&dy);assert(dx==0&&dy==0);
FrameInput state={480,272,0,0,0};FrameSample sample={0};
#define STEP() assert(!frame_input_update(&state,&sample,960,544,motion,back,records))
 STEP();assert(!count);sample.touched=1;sample.touch_x=100;sample.touch_y=200;STEP();assert(count==1&&records[0].action==0);
 sample.touch_x=101;STEP();assert(records[1].action==2&&records[1].x==101);
 sample.touched=0;sample.dx=50;STEP();assert(records[2].action==1&&records[2].x==101);assert(records[3].action==2&&records[3].x==151);
 sample.dx=10000;sample.dy=-10000;sample.click=1;STEP();assert(state.x==959&&state.y==0&&records[4].action==0);
 sample.dx=sample.dy=0;sample.back=1;STEP();STEP();assert(backs==1);sample.back=0;STEP();sample.back=1;STEP();assert(backs==2);
 sample.scroll=-1;STEP();assert(records[count-1].source==0x2002&&records[count-1].action==8&&records[count-1].scroll==-1);
 sample.scroll=0;sample.click=0;STEP();assert(!state.scroll_active&&records[count-1].scroll==0);
 int previous=count;sample.dx=NAN;assert(frame_input_update(&state,&sample,960,544,motion,back,records)==-1);assert(count==previous);return 0;
}
