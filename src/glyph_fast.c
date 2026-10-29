#include "glyph_fast.h"
#include "compat_libc.h"
#include "android_log.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
static void (ANDROID_ABI *glyph_original)(void*,void*,float,float,float);
static uint32_t *glyph_gc_stop;
static unsigned accepted,rejected,integer_rejected,page_rejected,data_rejected,capacity_rejected,gc_rejected;
static void *gp(void *p,unsigned n){uint32_t v;memcpy(&v,(char*)p+n,4);return (void*)(uintptr_t)v;}
static int32_t gi(void *p,unsigned n){int32_t v;memcpy(&v,(char*)p+n,4);return v;}
static float gf(void *p,unsigned n){float v;memcpy(&v,(char*)p+n,4);return v;}
static void *ga(void *a,unsigned *length){
 if(!a)return NULL;
 void *shape=gp(a,20);if(!shape)return NULL;
 int n=gi(shape,0);if(n<0)return NULL;
 *length=(unsigned)n;return gp(a,8);
}
void ANDROID_ABI glyph_fast(void *cache,void *glyph,float x,float y,float color){
 if(!glyph_gc_stop||*glyph_gc_stop){gc_rejected++;goto fallback;}
 if(!cache||!glyph){data_rejected++;goto fallback;}
 if(*((unsigned char*)cache+12)){integer_rejected++;goto fallback;}
 /* Multi-page caches also append a glyph index through a virtual method.
  * Keep that entire path original; no partial mutation before any guard. */
 if(gp(cache,52)){page_rejected++;goto fallback;}
 void *font=gp(cache,8),*data=font?gp(font,8):NULL;
 if(!data){data_rejected++;goto fallback;}
 int page=gi(glyph,64);unsigned ni=0,np=0,nv=0;
 int32_t *indices=ga(gp(cache,48),&ni);void *pages=ga(gp(cache,44),&np);
 if(page<0||!indices||!pages||(unsigned)page>=ni||(unsigned)page>=np){data_rejected++;goto fallback;}
 int index=indices[page];float *vertices=ga(gp(pages,(unsigned)page*4),&nv);
 if(!vertices||index<0||(unsigned)index>nv||nv-(unsigned)index<20){capacity_rejected++;goto fallback;}
 float sx=gf(data,60),sy=gf(data,64);
 if(!isfinite(x)||!isfinite(y)||!isfinite(sx)||!isfinite(sy)||fabsf(x)>100000||fabsf(y)>100000||fabsf(sx)>100||fabsf(sy)>100){data_rejected++;goto fallback;}
 float width=(float)gi(glyph,20)*sx,height=(float)gi(glyph,24)*sy;
 x+=(float)gi(glyph,44)*sx;y+=(float)gi(glyph,48)*sy;
 float x2=x+width,y2=y+height,u=gf(glyph,28),v=gf(glyph,32),u2=gf(glyph,36),v2=gf(glyph,40);
 float quad[20]={x,y,color,u,v, x,y2,color,u,v2, x2,y2,color,u2,v2, x2,y,color,u2,v};
 /* The original validates this same fixed 20-float span one element at a
  * time. All capacities are checked above, before the first write. */
 memcpy(vertices+index,quad,sizeof quad);indices[page]=index+20;accepted++;return;
 fallback:rejected++;glyph_original(cache,glyph,x,y,color);
}
void glyph_fast_report(void){
 android_log_print(4,"GlyphFast","fast=%u fallback=%u integer=%u pages=%u data=%u capacity=%u gc=%u",accepted,rejected,integer_rejected,page_rejected,data_rejected,capacity_rejected,gc_rejected);
 accepted=rejected=integer_rejected=page_rejected=data_rejected=capacity_rejected=gc_rejected=0;
}
int glyph_fast_configure(ElfImage *im){
 const char *name="_ZN3com8badlogic3gdx8graphics3g2d25BitmapFontCache$_addGlyphEPNS3_15BitmapFontCacheEPNS3_16BitmapFont$GlyphEfff";
 uint32_t value,size,slot,current,gc;
 const unsigned char sig[]={0x2d,0xe9,0xf0,0x43,0x81,0xb0,0x2d,0xed,0x10,0x8b};
 if(!im||!im->relocated||elf_image_symbol(im,name,&value,&size)||value!=0x1d9c68d||size!=1660||
 !elf_image_contains(im,value&~1u,sizeof sig,5)||memcmp(im->base+(value&~1u),sig,sizeof sig)||
 elf_image_symbol(im,"__$gc_stop_threads",&gc,&size)||!elf_image_contains(im,gc,4,6)||
 elf_image_relocation_target(im,name,22,&slot)||(slot&3)||!elf_image_contains(im,slot,4,2))return -1;
 memcpy(&current,im->base+slot,4);if(current!=(uint32_t)(uintptr_t)(im->base+value))return -1;
 glyph_original=(void*)(im->base+value);glyph_gc_stop=(void*)(im->base+gc);
 uint32_t target=(uint32_t)(uintptr_t)glyph_fast;memcpy(im->base+slot,&target,4);return 0;
}
