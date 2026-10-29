#include "launcher_settings.h"
#include "loading_screen.h"
#include "debugScreen.h"
#include "android_log.h"
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/display.h>
#include <psp2/power.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <string.h>
#include <stdio.h>
#define SETTINGS_PATH "ux0:data/spireandroid/launcher-settings.txt"
void launcher_show(void){
 int loaded=launcher_settings_load(SETTINGS_PATH,&launcher_settings);
 SceDisplayFrameBuf fb={.size=sizeof(fb)};PsvDebugScreenFont *font=psvDebugScreenGetFont();
 sceDisplayWaitVblankStart();
 if(sceDisplayGetFrameBuf(&fb,0)<0||!fb.base||fb.pitch!=960||fb.width!=960||fb.height!=544||!font||font->width!=8||font->height!=8||font->first!=0){android_log_print(5,"Launcher","Display unavailable; continuing with saved settings");return;}
 SceCtrlData pad;memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);unsigned previous=pad.buttons;
 SceTouchPanelInfo panel;int touch_ok=sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT,&panel)>=0&&panel.maxDispX>panel.minDispX&&panel.maxDispY>panel.minDispY;
 if(touch_ok)sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,SCE_TOUCH_SAMPLING_STATE_START);
 FILE *capture=fopen("ux0:data/spireandroid/enable-remote-capture.flag","rb");int capture_enabled=capture!=NULL;if(capture)fclose(capture);
 int page=0,row=0,down=0,dirty=1,done=0;unsigned tick=0;char error[64]="";
 if(loaded<0)strcpy(error,"Invalid settings: defaults restored.");
 while(!done){
  if(dirty){launcher_screen_draw(fb.base,font->glyphs,page,row,launcher_settings.effects,launcher_settings.diagnostics,launcher_settings.autoplay,error);
   if(capture_enabled){FILE *f=fopen("ux0:data/spireandroid/launcher-frame.rgba","wb");if(f){fwrite(fb.base,4,960*544,f);fclose(f);}}
   dirty=0;}
  sceKernelDelayThread(16667);if(++tick%60==0)sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
  memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);unsigned edge=pad.buttons&~previous;previous=pad.buttons;
  int rows=page?4:3,activate=!!(edge&SCE_CTRL_CROSS);
  if(edge&SCE_CTRL_UP){row=(row+rows-1)%rows;dirty=1;}
  if(edge&SCE_CTRL_DOWN){row=(row+1)%rows;dirty=1;}
  if(page&&(edge&(SCE_CTRL_LEFT|SCE_CTRL_RIGHT))&&row<2)activate=1;
  if(edge&SCE_CTRL_CIRCLE){if(page){page=0;row=1;dirty=1;}continue;}
  if(touch_ok){SceTouchData t={0};int touched=sceTouchPeek(SCE_TOUCH_PORT_FRONT,&t,1)>0&&t.reportNum;
   if(touched&&!down){int x=(t.report[0].x-panel.minDispX)*959/(panel.maxDispX-panel.minDispX),y=(t.report[0].y-panel.minDispY)*543/(panel.maxDispY-panel.minDispY);
    if(x>=140&&x<850&&y>=230&&y<230+rows*48){row=(y-230)/48;activate=1;dirty=1;}}
   down=touched;
  }
  if(!activate)continue;
  if(!page){if(row==2){LauncherSettings next=launcher_settings;next.autoplay=!next.autoplay;
    if(launcher_settings_save(SETTINGS_PATH,&next))strcpy(error,"Could not save auto-play. Press X to retry.");
    else{launcher_settings=next;error[0]=0;}dirty=1;
   }else if(row==1){page=1;row=0;dirty=1;}else{
    if(launcher_settings_save(SETTINGS_PATH,&launcher_settings)){strcpy(error,"Could not save settings. Press X to retry.");dirty=1;}
    else done=1;
   }}else{
   if(row==0)launcher_settings.effects=(launcher_settings.effects+1)%3;
   if(row==1)launcher_settings.diagnostics=!launcher_settings.diagnostics;
   if(row==2)launcher_settings=(LauncherSettings){0,0,0};
   if(row==3){page=0;row=1;}
   dirty=1;
  }
 }
 /* Consume the activation release so it cannot click the game's first menu. */
 do{sceKernelDelayThread(16667);memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);}while(pad.buttons&SCE_CTRL_CROSS);
 loading_screen_draw(fb.base,font->glyphs,"Preparing engine",0);
 android_log_print(4,"Launcher","effects=%d diagnostics=%d",launcher_settings.effects,launcher_settings.diagnostics);
}
