#include "text_submit.h"
#include "compat_libc.h"
#include "android_log.h"
#include <string.h>
#include <math.h>
/* Only the exact APK implementations below can use their audited layouts.
 * No retained vertices, reordered draws, altered colors or skipped render pass. */
static uintptr_t submit_expected[6];
static void **submit_batch_type;
static uint32_t *submit_gc_stop;
static unsigned submit_fast,submit_bypass,submit_reasons[11];
static uint32_t word(void*p,unsigned n){uint32_t v;memcpy(&v,(char*)p+n,4);return v;}
static void *ptr(void*p,unsigned n){return (void*)(uintptr_t)word(p,n);}
static int method_is(void *o,unsigned slot,uintptr_t target){
 if(!o||!target)return 0;
 void*t=ptr(o,0),*vt=t?ptr(t,4):NULL;
 return vt&&(uintptr_t)ptr(vt,slot)==target&&!*((unsigned char*)vt+slot+4);
}
static void *array_data(void *a,unsigned need){
 void *shape=a?ptr(a,20):NULL;
 return shape&&word(shape,0)>=need?ptr(a,8):NULL;
}
/* Diagnostic boundary only. Always call the original flush, guarded by the
 * audited exact class/method and active drawing state. No synthetic GPU work. */
int text_submit_flush_batch(void *batch){
 if(!submit_gc_stop||*submit_gc_stop||!submit_batch_type||!*submit_batch_type||
    !batch||ptr(batch,0)!=(char*)*submit_batch_type+0x38||
    !*((unsigned char*)batch+32)||!method_is(batch,184,submit_expected[5]))return 0;
 ((void (ANDROID_ABI *)(void*))submit_expected[5])(batch);return 1;
}
static int submit_page(void *cache,void *batch,const float *matrix){
 unsigned reason=0;
 if(!submit_gc_stop||*submit_gc_stop||!submit_batch_type||!*submit_batch_type)goto fallback;
 /* ClassHelpers::Alloc stores RuntimeType + 0x38 in the object header. */
 reason=9;if(!batch||ptr(batch,0)!=(char*)*submit_batch_type+0x38)goto fallback;
 reason=10;if(!method_is(cache,128,submit_expected[0]))goto fallback;
 reason=1;
 void *font=ptr(cache,8);
 if(!method_is(font,128,submit_expected[1]))goto fallback;
 reason=2;
 void *pages=ptr(cache,44),*indices=ptr(cache,48);
 if(!pages||!indices||!ptr(pages,20)||!ptr(indices,20)||word(ptr(pages,20),0)!=1||word(ptr(indices,20),0)!=1)goto fallback;
 reason=3;
 void *pa=array_data(pages,1),*ix=array_data(indices,1);if(!pa||!ix)goto fallback;
 reason=4;
 int count=(int)word(ix,0);if(count<0||count%20)goto fallback;
 reason=5;
 void *vertices=ptr(pa,0);if(count&&!array_data(vertices,(unsigned)count))goto fallback;
 reason=6;
 void *regions=ptr(font,12);
 if(!method_is(regions,72,submit_expected[2])||word(regions,12)!=1)goto fallback;
 reason=7;
 void *items=array_data(ptr(regions,8),1);if(!items)goto fallback;
 void *region=ptr(items,0);if(!method_is(region,56,submit_expected[3]))goto fallback;
 reason=8;
 void *texture=ptr(region,8);if(!texture)goto fallback;
 /* Exact SpriteBatch.draw synchronously copies source floats into its own
  * vertex buffer (APK0x1db0c2c/0x1db0c78). Restore source XY before returning.
  * Bound the stack storage; no allocations or retained managed pointers. */
 float saved_xy[2048];float *v=count?array_data(vertices,(unsigned)count):NULL;
 if(matrix){
  if(count>5120||!*((unsigned char*)batch+32)||ptr(batch,64))goto fallback;
  for(unsigned i=0;i<16;i++)if(!isfinite(matrix[i])||fabsf(matrix[i])>1000000)goto fallback;
  if(matrix[2]||matrix[3]||matrix[6]||matrix[7]||matrix[8]||matrix[9]||matrix[10]!=1||matrix[11]||matrix[14]||matrix[15]!=1)goto fallback;
  for(int i=0;i<count;i+=5){float x=v[i],y=v[i+1];if(!isfinite(x)||!isfinite(y)||fabsf(x)>100000||fabsf(y)>100000)goto fallback;}
  for(int i=0,j=0;i<count;i+=5,j+=2){float x=v[i],y=v[i+1];saved_xy[j]=x;saved_xy[j+1]=y;v[i]=x*matrix[0]+y*matrix[4]+matrix[12];v[i+1]=x*matrix[1]+y*matrix[5]+matrix[13];}
 }
 if(count)((void (ANDROID_ABI *)(void*,void*,void*,int,int))submit_expected[4])(batch,texture,vertices,0,count);
 if(matrix)for(int i=0,j=0;i<count;i+=5,j+=2){v[i]=saved_xy[j];v[i+1]=saved_xy[j+1];}
 submit_fast++;return 1;
 fallback:submit_reasons[reason]++;submit_bypass++;return 0;
}
int text_submit_single_page(void *cache,void *batch){return submit_page(cache,batch,NULL);}
int text_submit_rotated_page(void *cache,void *batch,void *matrix){
 void *a=matrix?ptr(matrix,8):NULL;float *v=a?array_data(a,16):NULL;
 if(!v)return 0;
 return submit_page(cache,batch,v);
}
void text_submit_report(void){android_log_print(4,"TextSubmit","submit_fast=%u submit_bypass=%u",submit_fast,submit_bypass);android_log_print(4,"TextSubmitGuard","entry=%u font=%u pages=%u arrays=%u count=%u vertices=%u regions=%u region=%u texture=%u batch_type=%u cache_method=%u",submit_reasons[0],submit_reasons[1],submit_reasons[2],submit_reasons[3],submit_reasons[4],submit_reasons[5],submit_reasons[6],submit_reasons[7],submit_reasons[8],submit_reasons[9],submit_reasons[10]);memset(submit_reasons,0,sizeof(submit_reasons));submit_fast=submit_bypass=0;}
static const struct {const char *name;uint32_t value,size,n;unsigned char bytes[8];} targets[6]={
{"_ZN3com8badlogic3gdx8graphics3g2d26BitmapFontCache$_draw_VoidEPNS3_15BitmapFontCacheEPNS3_5BatchE",0x1d9b455,764,8,{0x2d,0xe9,0xf0,0x4f,0x87,0xb0,0x6,0x91}},
{"_ZN3com8badlogic3gdx8graphics3g2d46BitmapFont$_get_Regions_Array$1_$TextureRegionEPNS3_10BitmapFontE",0x1d9841d,4,4,{0xc0,0x68,0x70,0x47}},
{"_ZN3com8badlogic3gdx5utils14Array$1$_get_TIPNS1_8graphics3g2d13TextureRegionEEET_PNS2_7Array$1IS8_EEi",0x292d225,236,8,{0x70,0xb5,0x86,0xb0,0x6,0x46,0x34,0x48}},
{"_ZN3com8badlogic3gdx8graphics3g2d34TextureRegion$_get_Texture_TextureEPNS3_13TextureRegionE",0x1db4d09,4,4,{0x80,0x68,0x70,0x47}},
{"_ZN3com8badlogic3gdx8graphics3g2d22SpriteBatch$_draw_VoidEPNS3_11SpriteBatchEPNS2_7TextureEPN6System6ArrayTIfLi1EEEii",0x1db0bb5,340,8,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x82,0x46}},
{"_ZN3com8badlogic3gdx8graphics3g2d23SpriteBatch$_flush_VoidEPNS3_11SpriteBatchE",0x1db1839,720,8,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x4,0x46}}
};
int text_submit_configure(ElfImage *im){
 if(!im||!im->relocated)return -1;
 uint32_t v[6],size,type,stop;
 for(unsigned i=0;i<6;i++)if(elf_image_symbol(im,targets[i].name,v+i,&size)||v[i]!=targets[i].value||size!=targets[i].size||!elf_image_contains(im,v[i]&~1u,targets[i].n,5)||memcmp(im->base+(v[i]&~1u),targets[i].bytes,targets[i].n))return -1;
 if(elf_image_symbol(im,"_ZZN6System8Internal14GetRuntimeTypeIPN3com8badlogic3gdx8graphics3g2d11SpriteBatchEEEPNS0_11RuntimeTypeEvE6cached",&type,&size)||size!=4||!elf_image_contains(im,type,4,6))return -1;
 if(elf_image_symbol(im,"__$gc_stop_threads",&stop,&size)||size!=4||!elf_image_contains(im,stop,4,6))return -1;
 for(unsigned i=0;i<6;i++)submit_expected[i]=(uintptr_t)(im->base+v[i]);
 submit_batch_type=(void*)(im->base+type);submit_gc_stop=(void*)(im->base+stop);return 0;
}
