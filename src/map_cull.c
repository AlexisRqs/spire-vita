#include "map_cull.h"
#include "android_log.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
uint32_t map_original_resume;
extern void map_dot_original(void*,void*);
static float *map_scroll,*map_offset,*map_scale;
static void **map_texture;
static unsigned char *map_init[3];
static uintptr_t map_draw;
static uint32_t *map_gc_stop;
static unsigned map_skipped,map_kept;
static void *map_ptr(void *p,unsigned n){uint32_t v;memcpy(&v,(char*)p+n,4);return (void*)(uintptr_t)v;}
static float map_float(void *p,unsigned n){float v;memcpy(&v,(char*)p+n,4);return v;}
static float *map_matrix(void *m){
 if(!m)return NULL;
 void *a=map_ptr(m,8);if(!a)return NULL;
 uint32_t *shape=map_ptr(a,20);if(!shape||*shape!=16)return NULL;
 return map_ptr(a,8);
}
void map_dot_cull(void *dot,void *batch){
 if(!map_gc_stop||*map_gc_stop||!dot||!batch||!map_scroll||!map_offset||!map_scale||!map_texture||!*map_texture)goto keep;
 for(unsigned i=0;i<3;i++)if(!map_init[i]||!*map_init[i])goto keep;
 void *type=map_ptr(batch,0),*vt=map_ptr(type,4);
 if((uintptr_t)map_ptr(vt,88)!=map_draw||*((unsigned char*)vt+92)||map_ptr(batch,64)||!*((unsigned char*)batch+32))goto keep;
 float *t=map_matrix(map_ptr(batch,36)),*p=map_matrix(map_ptr(batch,40));if(!t||!p)goto keep;
 for(unsigned i=0;i<16;i++)if(t[i]!=(i%5==0?1.0f:0.0f))goto keep;
 /* Only ordinary orthographic projections. Other render targets/shaders use
  * the original function. Leave a generous clip margin for float rounding. */
 for(unsigned i=0;i<16;i++){
  if(!isfinite(p[i]))goto keep;
  if(i!=0&&i!=5&&i!=10&&i!=12&&i!=13&&i!=14&&i!=15&&p[i]!=0)goto keep;
 }
 if(p[0]<=0||p[5]<=0||p[15]!=1)goto keep;
 float x=map_float(dot,8),y=map_float(dot,12),scale=*map_scale;
 if(!isfinite(x)||!isfinite(y)||!isfinite(scale)||!isfinite(map_float(dot,16)))goto keep;
 /* Original draws a 16x16 sprite around origin (8,8). The sum of the half
  * extents encloses every rotation; preserve original Y addition order. */
 x=(x-8.0f)+8.0f;y=(((y-8.0f)+*map_scroll)+*map_offset)+8.0f;
 float r=16.0f*fabsf(scale)+2.0f;
 float cx=p[0]*x+p[12],cy=p[5]*y+p[13],rx=p[0]*r,ry=p[5]*r;
 if(!isfinite(cx)||!isfinite(cy)||!isfinite(rx)||!isfinite(ry))goto keep;
 if(cx+rx < -1.01f||cx-rx > 1.01f||cy+ry < -1.01f||cy-ry > 1.01f){map_skipped++;return;}
 keep:map_kept++;map_dot_original(dot,batch);
}
void map_cull_report(void){android_log_print(4,"MapCull","skipped=%u kept=%u",map_skipped,map_kept);map_skipped=map_kept=0;}
int map_cull_configure(ElfImage *im){
 const char *names[]={
 "_ZN3com8megacrit9cardcrawl3vfx19MapDot$_render_VoidEPNS2_6MapDotEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE",
 "_ZN3com8megacrit9cardcrawl7screens16DungeonMapScreen7offsetYE",
 "_ZN3com8megacrit9cardcrawl3vfx6MapDot8OFFSET_YE",
 "_ZN3com8megacrit9cardcrawl4core8Settings5scaleE",
 "_ZN3com8megacrit9cardcrawl7helpers11ImageMaster9MAP_DOT_1E",
 "_ZZN6System8Internal12ClassHelpers9ClassCtorIN3com8megacrit9cardcrawl4core8SettingsEEEvvE11_initalized",
 "_ZZN6System8Internal12ClassHelpers9ClassCtorIN3com8megacrit9cardcrawl7screens16DungeonMapScreenEEEvvE11_initalized",
 "_ZZN6System8Internal12ClassHelpers9ClassCtorIN3com8megacrit9cardcrawl7helpers11ImageMasterEEEvvE11_initalized",
 "_ZN3com8badlogic3gdx8graphics3g2d22SpriteBatch$_draw_VoidEPNS3_11SpriteBatchEPNS2_7TextureEfffffffffiiiibb",
 "__$gc_stop_threads"};
 if(!im||!im->relocated)return -1;
 uint32_t v[10],size;
 for(unsigned i=0;i<10;i++)if(elf_image_symbol(im,names[i],v+i,&size)||!elf_image_contains(im,v[i]&~1u,i>=5&&i<=7?1:4,i==0||i==8?5:6))return -1;
 const unsigned char sig[]={0x2d,0xe9,0xf0,0x41,0x8e,0xb0,0x05,0x46};
 if((v[0]&3)!=1||memcmp(im->base+(v[0]&~1u),sig,8))return -1;
 map_scroll=(void*)(im->base+v[1]);map_offset=(void*)(im->base+v[2]);map_scale=(void*)(im->base+v[3]);map_texture=(void*)(im->base+v[4]);
 for(unsigned i=0;i<3;i++)map_init[i]=im->base+v[5+i];
 map_draw=(uintptr_t)(im->base+v[8]);map_gc_stop=(void*)(im->base+v[9]);
 map_original_resume=(uint32_t)(uintptr_t)(im->base+(v[0]&~1u)+8)|1;
 const unsigned char jump[]={0xdf,0xf8,0,0xf0};uint32_t target=(uint32_t)(uintptr_t)map_dot_cull;
 memcpy(im->base+(v[0]&~1u),jump,4);memcpy(im->base+(v[0]&~1u)+4,&target,4);return 0;
}
