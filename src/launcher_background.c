#include "launcher_background.h"
#include "launcher_flow.h"
#include "launcher_splash.h"
#include "launcher_settings.h"
#include "loading_screen.h"
#include "game_hooks.h"
#include "android_log.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/power.h>
#include <psp2/kernel/processmgr.h>
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static LauncherFlow flow;
static LauncherSettings choices;
static int row,down,touch_ok;
static unsigned previous,tick,stage;
static int menu_prepared,release_gate,splash_available,splash_skip;
static uint64_t splash_at;
static SceTouchPanelInfo panel;
static char phase[64]="Preparing engine",error[64];
static uint64_t started,requested_at,ready_at;
void launcher_background_prepare(void){
 int result=launcher_settings_load("ux0:data/spireandroid/launcher-settings.txt",&launcher_settings);
 choices=launcher_settings;flow=(LauncherFlow){.active=1,.requested=choices.autoplay};row=down=0;tick=stage=0;menu_prepared=release_gate=0;strcpy(phase,"Preparing engine");
 splash_available=launcher_splash_prepare();splash_at=0;splash_skip=0;
 error[0]=0;if(result<0)strcpy(error,"Invalid settings: defaults restored.");
 started=sceKernelGetProcessTimeWide();requested_at=choices.autoplay?started:0;ready_at=0;
 SceCtrlData pad={0};sceCtrlPeekBufferPositive(0,&pad,1);previous=pad.buttons;
 touch_ok=sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT,&panel)>=0&&panel.maxDispX>panel.minDispX&&panel.maxDispY>panel.minDispY;
 if(touch_ok)sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,SCE_TOUCH_SAMPLING_STATE_START);
}
void launcher_background_cancel(void){pthread_mutex_lock(&lock);flow.active=0;launcher_splash_release();pthread_mutex_unlock(&lock);}
int launcher_background_active(void){pthread_mutex_lock(&lock);int active=flow.active;pthread_mutex_unlock(&lock);return active;}
void launcher_background_ready(void){pthread_mutex_lock(&lock);menu_prepared=1;pthread_mutex_unlock(&lock);}
int launcher_background_needs_frame(void){pthread_mutex_lock(&lock);int pending=flow.active&&menu_prepared&&!flow.ready;pthread_mutex_unlock(&lock);return pending;}
void launcher_background_frame_ready(void){
 pthread_mutex_lock(&lock);if(menu_prepared){flow.ready=1;ready_at=sceKernelGetProcessTimeWide();stage=9;strcpy(phase,"Ready to play");}pthread_mutex_unlock(&lock);
}
void launcher_background_stage(unsigned next,const char *label){
 pthread_mutex_lock(&lock);if(next>=stage&&next<9&&!flow.ready){stage=next;snprintf(phase,sizeof phase,"%s",label);}pthread_mutex_unlock(&lock);
}
void launcher_background_phase(const char *s){pthread_mutex_lock(&lock);snprintf(phase,sizeof phase,"%s",s);pthread_mutex_unlock(&lock);}
void launcher_background_tick(void){
 pthread_mutex_lock(&lock);
 if(!flow.active){pthread_mutex_unlock(&lock);return;}
 if(++tick%60==0)sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
 SceCtrlData pad={0};if(sceCtrlPeekBufferPositive(0,&pad,1)<0){pthread_mutex_unlock(&lock);return;}
 unsigned edge=pad.buttons&~previous;previous=pad.buttons;
 if(splash_at){
  if(edge&SCE_CTRL_CROSS)splash_skip=1;
  if(touch_ok){SceTouchData t={0};int touched=sceTouchPeek(SCE_TOUCH_PORT_FRONT,&t,1)>0&&t.reportNum;if(touched&&!down)splash_skip=1;down=touched;}
  pthread_mutex_unlock(&lock);return;
 }
 int rows=flow.page?4:3,activate=!!(edge&SCE_CTRL_CROSS);
 if(edge&SCE_CTRL_UP)row=(row+rows-1)%rows;
 if(edge&SCE_CTRL_DOWN)row=(row+1)%rows;
 if(flow.page&&(edge&(SCE_CTRL_LEFT|SCE_CTRL_RIGHT))&&row<2)activate=1;
 if(edge&SCE_CTRL_CIRCLE){if(flow.page){flow.page=0;row=1;}else flow.requested=0;activate=0;}
 if(touch_ok){SceTouchData t={0};int touched=sceTouchPeek(SCE_TOUCH_PORT_FRONT,&t,1)>0&&t.reportNum;
  if(touched&&!down){int x=(t.report[0].x-panel.minDispX)*959/(panel.maxDispX-panel.minDispX),y=(t.report[0].y-panel.minDispY)*543/(panel.maxDispY-panel.minDispY);
   if(x>=140&&x<850&&y>=230&&y<230+rows*48){row=(y-230)/48;activate=1;}}
  down=touched;
 }
 if(activate){
  if(!flow.page){if(row==2){
   LauncherSettings next=choices;next.autoplay=!next.autoplay;
   if(launcher_settings_save("ux0:data/spireandroid/launcher-settings.txt",&next))strcpy(error,"Could not save auto-play. Press X to retry.");
   else{choices=next;flow.requested=choices.autoplay;requested_at=sceKernelGetProcessTimeWide();error[0]=0;}
  }else if(row==1){flow.page=1;row=0;}else{flow.requested=1;requested_at=sceKernelGetProcessTimeWide();}}
  else if(row==0)choices.effects=(choices.effects+1)%3;
  else if(row==1)choices.diagnostics=!choices.diagnostics;
  else if(row==2){choices=(LauncherSettings){0,0,0};flow.requested=0;}
  else{flow.page=0;row=1;}
 }
 pthread_mutex_unlock(&lock);
}
void launcher_background_draw(uint32_t *pixels,const unsigned char *font){
 pthread_mutex_lock(&lock);
 if(flow.active&&splash_at){launcher_splash_draw(pixels,(unsigned)((sceKernelGetProcessTimeWide()-splash_at)/1000));}
 else if(flow.active){
  launcher_screen_draw(pixels,font,flow.page,row,choices.effects,choices.diagnostics,choices.autoplay,error);
  launcher_loading_status_draw(pixels,font,flow.page,row,flow.ready,flow.requested,phase,stage,(unsigned)((sceKernelGetProcessTimeWide()-started)/1000));
 }
 pthread_mutex_unlock(&lock);
}
int launcher_background_gate(int held){
 pthread_mutex_lock(&lock);int blocked=flow.active||release_gate,handoff=0,apply=0;
 /* Only the validating Cross/front touch can hold this release latch. */
 if(!flow.active&&release_gate&&!held){release_gate=0;android_log_print(4,"Launcher","Game input released");}
 if(splash_at){
  if(splash_skip||sceKernelGetProcessTimeWide()-splash_at>=LAUNCHER_SPLASH_DURATION_MS*1000ull){
   flow.active=0;release_gate=1;handoff=1;splash_at=0;launcher_splash_release();
  }
 }else if(launcher_flow_handoff(&flow)){
  if(launcher_settings_save("ux0:data/spireandroid/launcher-settings.txt",&choices)){
   strcpy(error,"Could not save settings. Press X to retry.");flow.requested=0;
  }else{
   launcher_settings=choices;apply=1;
   if(splash_available){splash_at=sceKernelGetProcessTimeWide();splash_skip=0;}
   else{flow.active=0;release_gate=1;handoff=1;}
   android_log_print(4,"Launcher","Play accepted splash=%d requested_when_ready=%d request_to_response_us=%llu",splash_available,requested_at>=ready_at,(unsigned long long)(sceKernelGetProcessTimeWide()-requested_at));
  }
 }
 pthread_mutex_unlock(&lock);
 if(apply)game_settings_apply_launcher();
 if(handoff){boot_display_stop();android_log_print(4,"Launcher","Ready handoff effects=%d diagnostics=%d",launcher_settings.effects,launcher_settings.diagnostics);}
 return blocked;
}
