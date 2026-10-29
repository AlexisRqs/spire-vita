#include "launcher_splash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Original logos, predecoded offline. No texture uploads on Play. */
static unsigned char *logos;
void launcher_splash_release(void){free(logos);logos=NULL;}
int launcher_splash_prepare(void){
 launcher_splash_release();FILE *f=fopen("app0:launcher-splashes.bin","rb");if(!f)return 0;
 const size_t size=2*640*360*4;char magic[8];
 int ok=fread(magic,1,8,f)==8&&!memcmp(magic,"SPLASH01",8);
 if(ok)logos=malloc(size);
 ok=ok&&logos&&fread(logos,1,size,f)==size&&fgetc(f)==EOF&&!ferror(f);
 fclose(f);if(!ok)launcher_splash_release();return ok;
}
void launcher_splash_draw(uint32_t *pixels,unsigned ms){
 for(unsigned i=0;i<960*544;i++)pixels[i]=0xff000000;
 if(!logos||ms>=LAUNCHER_SPLASH_DURATION_MS)return;
 unsigned index=ms/2000,t=ms%2000;
 unsigned alpha=t<250?t*255/250:t>1750?(2000-t)*255/250:255;
 const unsigned char *src=logos+index*640*360*4;
 for(unsigned y=0;y<360;y++)for(unsigned x=0;x<640;x++){
  const unsigned char *c=src+(y*640+x)*4;
  pixels[(y+92)*960+x+160]=0xff000000|((c[0]*alpha/255))|((c[1]*alpha/255)<<8)|((c[2]*alpha/255)<<16);
 }
}
