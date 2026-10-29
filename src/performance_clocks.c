#include "android_log.h"
#include <psp2/power.h>
#include <stdio.h>
void performance_clocks_start(void){
 int (*get[])(void)={scePowerGetArmClockFrequency,scePowerGetBusClockFrequency,scePowerGetGpuClockFrequency,scePowerGetGpuXbarClockFrequency};
 int (*set[])(int)={scePowerSetArmClockFrequency,scePowerSetBusClockFrequency,scePowerSetGpuClockFrequency,scePowerSetGpuXbarClockFrequency};
 const char *names[]={"CPU","BUS","GPU","XBAR"};const int floor[]={444,222,222,166};
 for(unsigned i=0;i<4;i++){
  int before=get[i](),status=0;
  /* Respect higher user clocks; do not fight plugins by retrying each frame. */
  if(before>=0&&before<floor[i])status=set[i](floor[i]);
  android_log_print(4,"Clocks","%s before=%d requested_floor=%d after=%d status=%d",names[i],before,floor[i],get[i](),status);
 }
}

typedef struct {int cpu,gpu,xbar,bus;} ShellClocks;
extern int psvsGetClockFrequency(ShellClocks *);
void performance_clocks_report(void){
 ShellClocks c={0};int r=psvsGetClockFrequency(&c);
 if(!r)android_log_print(4,"ShellClocks","cpu=%d gpu=%d bus=%d xbar=%d",c.cpu,c.gpu,c.bus,c.xbar);
 else android_log_print(5,"ShellClocks","unavailable=%d",r);
 android_log_print(4,"ClockSample","cpu=%d bus=%d gpu=%d xbar=%d",scePowerGetArmClockFrequency(),scePowerGetBusClockFrequency(),scePowerGetGpuClockFrequency(),scePowerGetGpuXbarClockFrequency());
}
