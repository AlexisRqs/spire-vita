#include "loading_screen.h"
#include <stdio.h>
#include <string.h>
static void rect(uint32_t *p,int x,int y,int w,int h,uint32_t c){
 for(int j=y;j<y+h&&j<544;j++)for(int i=x;i<x+w&&i<960;i++)if(i>=0&&j>=0)p[j*960+i]=c;
}
static void text(uint32_t*p,const unsigned char*font,int x,int y,int scale,const char*s,uint32_t color){
 for(;*s;s++,x+=8*scale){unsigned c=(unsigned char)*s;
  for(int j=0;j<8;j++)for(int i=0;i<8;i++)if(font[c*8+j]&(0x80>>i))rect(p,x+i*scale,y+j*scale,scale,scale,color);
 }
}
void loading_screen_draw(uint32_t *p,const unsigned char *font,const char *stage,unsigned ms){
 const uint32_t bg=0xff191410,gold=0xff85b7ca,white=0xffe5e9ed,muted=0xff9b9487;
 for(unsigned i=0;i<960*544;i++)p[i]=bg;
 /* A small geometric spire, no game texture upload or shader compilation. */
 rect(p,92,155,7,213,gold);rect(p,72,320,48,4,gold);
 for(int y=0;y<75;y++)rect(p,95-y/5,126+y,1+2*(y/5),1,gold);
 rect(p,69,367,54,3,gold);rect(p,78,288,35,3,gold);
 text(p,font,164,180,4,"SLAY THE SPIRE",white);
 text(p,font,166,231,2,"PS VITA",gold);
 rect(p,166,282,610,1,0xff40362a);
 text(p,font,166,314,2,stage,white);
 char time[64];snprintf(time,sizeof(time),"LOADING  %u:%02u",ms/60000,(ms/1000)%60);
 text(p,font,166,365,2,time,muted);
 for(unsigned i=0;i<12;i++)rect(p,166+i*22,411,12,4,i==(ms/110)%12?gold:0xff40362a);
 text(p,font,166,466,1,"Game resources are being prepared.",muted);
}

void launcher_screen_draw(uint32_t *p,const unsigned char *font,int page,int selected,int effects,int diagnostics,int autoplay,const char *error){
 const uint32_t bg=0xff191410,gold=0xff85b7ca,white=0xffe5e9ed,muted=0xff9b9487;
 for(unsigned i=0;i<960*544;i++)p[i]=bg;
 text(p,font,140,80,4,"SLAY THE SPIRE",white);
 text(p,font,142,132,2,"PS VITA",gold);
 text(p,font,142,190,2,page?"PORT SETTINGS":"READY TO PLAY",muted);
 char effect[64],diag[64];const char *values[]={"Game setting","Off","On"};
 snprintf(effect,sizeof(effect),"Particles: %s",values[effects]);
 snprintf(diag,sizeof(diag),"Detailed metrics: %s",diagnostics?"On":"Off");
 const char *home[]={"Play","Settings",autoplay?"[x] Auto-play when ready":"[ ] Auto-play when ready"};const char *options[]={effect,diag,"Restore defaults","Back"};
 for(int i=0;i<(page?4:3);i++){
  if(i==selected)rect(p,140,230+i*48,710,40,0xff40362a);
  text(p,font,156,242+i*48,2,page?options[i]:home[i],i==selected?gold:white);
 }
 text(p,font,142,440,1,page?"Cards and text: native resolution. Clock settings: PSVshell.":"Your settings are saved when you enter the game.",muted);
 text(p,font,142,470,1,"D-pad: select   X: confirm   O: back   Touch supported",muted);
 if(error&&*error)text(p,font,142,505,1,error,gold);
}

void launcher_loading_status_draw(uint32_t *p,const unsigned char *font,int page,int selected,int ready,int requested,const char *phase,unsigned stage,unsigned ms){
 const uint32_t bg=0xff191410,gold=0xff85b7ca,white=0xffe5e9ed,muted=0xff9b9487;
 if(stage>9)stage=9;
 char status[96];snprintf(status,sizeof status,"%s  |  %u:%02u",ready?"Ready to play":phase,ms/60000,(ms/1000)%60);
 rect(p,140,150,720,30,bg);text(p,font,142,153,1,status,muted);
 /* Completed milestones, not an invented byte/time percentage. */
 rect(p,142,169,708,8,0xff40362a);
 rect(p,142,169,ready?708:(int)(stage*708/9),8,gold);
 if(!ready){int x=142+(int)((ms/40)%700);rect(p,x,169,8,8,white);}
 if(!page){
  rect(p,140,186,720,28,bg);text(p,font,142,190,2,ready?"READY TO PLAY":"LOADING",muted);
  rect(p,140,230,710,40,selected==0?0xff40362a:bg);
  text(p,font,156,242,2,requested?"Play requested":ready?"Play":"Play when ready",selected==0?gold:white);
 }
 rect(p,140,436,720,18,bg);
 text(p,font,142,440,1,page?"Loading continues here. Your changes apply when you enter the game.":requested?"The game will open when ready, after you leave Settings.":ready?"Press X on Play to start. X again skips the startup logos.":"Loading starts automatically. You can change Settings while you wait.",muted);
}
