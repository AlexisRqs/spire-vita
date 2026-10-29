#include "vita_frame.h"
#include "frame_input.h"
#include "android_frame_loop.h"
#include "android_log.h"
#include "keyboard.h"
#include "launcher_background.h"
#include "audio_hooks.h"
#include "native_controller.h"
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <string.h>
static struct {
 AndroidApp32 *app;EngineInput input;EngineBack back;FrameInput pointer;
 SceTouchPanelInfo panel,rear_panel;int rear;uint64_t last_frame;int touching,cursor;
} state;
static void motion(const AndroidMotion *event,void *unused){(void)unused;state.input(state.app,event);}
static void back(void *unused){(void)unused;if(state.back)state.back();}
int vita_frame_pointer(float *x,float *y){
 *x=state.pointer.x;*y=state.pointer.y;return state.cursor&&!keyboard_active();
}
static int pump(int timeout,void *unused){
 (void)unused;audio_hooks_poll();uint64_t now=sceKernelGetProcessTimeWide();
 uint64_t due=state.last_frame+16667;
 if(now<due){
  if(timeout==0)return -3;
  uint64_t wait=due-now;
  if(timeout>0&&(uint64_t)timeout*1000<wait){sceKernelDelayThread(timeout*1000);return -3;}
  sceKernelDelayThread((unsigned)wait);now=sceKernelGetProcessTimeWide();
 }
 float seconds=state.last_frame?(now-state.last_frame)/1000000.0f:1.0f/60;
 if(seconds>0.1f)seconds=0.1f;
 state.last_frame=now;
 SceCtrlData pad;memset(&pad,0,sizeof(pad));pad.lx=pad.ly=pad.rx=pad.ry=128;
 if(sceCtrlPeekBufferPositive(0,&pad,1)<0)return -4;
 if((pad.buttons&(SCE_CTRL_SELECT|SCE_CTRL_START))==(SCE_CTRL_SELECT|SCE_CTRL_START)){
  android_log_print(4,"Input","Select+Start: exit requested");sceKernelExitProcess(0);
 }
 FrameSample sample={0};sample.click=!!(pad.buttons&SCE_CTRL_CROSS);sample.back=!!(pad.buttons&SCE_CTRL_CIRCLE);
 frame_pointer_delta(pad.lx,pad.ly,pad.rx,pad.ry,
  !!(pad.buttons&SCE_CTRL_RIGHT)-!!(pad.buttons&SCE_CTRL_LEFT),
  !!(pad.buttons&SCE_CTRL_DOWN)-!!(pad.buttons&SCE_CTRL_UP),seconds,&sample.dx,&sample.dy);
 sample.scroll=(!!(pad.buttons&SCE_CTRL_LTRIGGER)-!!(pad.buttons&SCE_CTRL_RTRIGGER));
 if(state.touching){
  SceTouchData touch;memset(&touch,0,sizeof(touch));
  if(sceTouchPeek(SCE_TOUCH_PORT_FRONT,&touch,1)>0&&touch.reportNum){
   sample.touched=1;
   sample.touch_x=(touch.report[0].x-state.panel.minDispX)*959.0f/(state.panel.maxDispX-state.panel.minDispX);
   sample.touch_y=(touch.report[0].y-state.panel.minDispY)*543.0f/(state.panel.maxDispY-state.panel.minDispY);
  }
 }
 uint32_t native_buttons=pad.buttons;
 if(state.rear){
  SceTouchData touch;memset(&touch,0,sizeof(touch));
  if(sceTouchPeek(SCE_TOUCH_PORT_BACK,&touch,1)>0){
   int w=state.rear_panel.maxDispX-state.rear_panel.minDispX;
   int h=state.rear_panel.maxDispY-state.rear_panel.minDispY;
   for(unsigned i=0;i<touch.reportNum&&i<sizeof(touch.report)/sizeof(touch.report[0]);i++){
    int x=touch.report[i].x-state.rear_panel.minDispX,y=touch.report[i].y-state.rear_panel.minDispY;
    if(y>=0&&y<h/2&&x>=0&&x<=w){
     if(x<w/3)native_buttons|=VITA_VIRTUAL_L2;
     else if(x>2*w/3)native_buttons|=VITA_VIRTUAL_R2;
    }
   }
  }
 }
 int held=sample.touched||native_controller_input_held(native_buttons,pad.lx,pad.ly,pad.rx,pad.ry);
 int launch_gate=launcher_background_gate(sample.click||sample.touched);
 int gated=keyboard_gate_input(held)||launch_gate;
 static uint64_t trace_time;static unsigned trace_count;
 if(trace_count<32&&now-trace_time>500000&&(pad.buttons||sample.dx||sample.dy)){
  android_log_print(4,"PadTrace","buttons=0x%x lx=%u ly=%u rx=%u ry=%u gated=%d click=%d",(unsigned)pad.buttons,pad.lx,pad.ly,pad.rx,pad.ry,gated,sample.click);
  trace_time=now;trace_count++;
 }
 native_controller_sample(native_buttons,pad.lx,pad.ly,pad.rx,pad.ry,gated);
 if(native_controller_active()){
  sample.dx=sample.dy=sample.scroll=0;sample.click=sample.back=0;state.cursor=0;
 }
 if(gated){
  memset(&sample,0,sizeof(sample));
 }
 if(sample.touched)state.cursor=0;
 else if(sample.dx||sample.dy||sample.click)state.cursor=1;
 return frame_input_update(&state.pointer,&sample,960,544,motion,back,NULL)?-4:-1;
}
int vita_frame_configure(AndroidApp32 *app,EngineInput input,EngineBack escape){
 if(!app||!app->user||!input)return -1;
 memset(&state,0,sizeof(state));state.app=app;state.input=input;state.back=escape;
 state.pointer.x=480;state.pointer.y=272;
 if(sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG)<0)return -1;
 if(sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT,&state.panel)>=0&&state.panel.maxDispX>state.panel.minDispX&&state.panel.maxDispY>state.panel.minDispY)
  state.touching=sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,SCE_TOUCH_SAMPLING_STATE_START)>=0;
 if(sceTouchGetPanelInfo(SCE_TOUCH_PORT_BACK,&state.rear_panel)>=0&&state.rear_panel.maxDispX>state.rear_panel.minDispX&&state.rear_panel.maxDispY>state.rear_panel.minDispY)
  state.rear=sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK,SCE_TOUCH_SAMPLING_STATE_START)>=0;
 android_log_print(4,"Input","Rear touch=%d: upper left/right thirds = L2/R2",state.rear);
 android_log_print(4,"Input","Front touch sampling=%d; D-pad/both sticks=pointer, Cross=click, Circle=back",state.touching);
 return android_frame_loop_configure(pump,NULL);
}
void vita_frame_shutdown(void){
 android_frame_loop_configure(NULL,NULL);
 if(state.rear)sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK,SCE_TOUCH_SAMPLING_STATE_STOP);
 if(state.touching)sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,SCE_TOUCH_SAMPLING_STATE_STOP);
 memset(&state,0,sizeof(state));
}
