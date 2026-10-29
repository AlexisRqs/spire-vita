#include "text_fast.h"
#include "text_memo.h"
#include "text_submit.h"
#include "scene_metrics.h"
#include "compat_libc.h"
#include "android_log.h"
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
uint32_t text_original_resume;
extern ANDROID_ABI void text_original(void*,void*,void*,float,float,float,float,float,int,void*);
static void **layout_slot,**restore_slot,**matrix_slot,**vector_slot;
static unsigned char *text_initialized;
static uintptr_t batch_setter;
static int (ANDROID_ABI *round_float)(float);
static double (ANDROID_ABI *round_double)(double,int);
static unsigned fast_count,fallback_count,prepared_count;
static int text_profile_enabled,rotation_enabled,integer_enabled;
static unsigned rotation_count;
void text_integer_enable(int enabled){integer_enabled=enabled!=0;}
void text_rotation_enable(int enabled){rotation_enabled=enabled!=0;}
static uint64_t text_total_us,text_setup_us,text_memo_us,text_draw_us,text_fallback_us;
static unsigned text_profile_calls;
void text_profile_enable(int enabled){text_profile_enabled=enabled;}

enum {TF_ARGUMENT,TF_ANGLE,TF_INIT,TF_COORD,TF_SETTER,TF_SHADER,TF_INACTIVE,TF_INTEGER,TF_TRANSFORM,TF_RESTORE,TF_DATA,TF_REASONS};
static unsigned fallback_reason[TF_REASONS];
static uintptr_t font_draw_expected,cache_add_expected;
static void (ANDROID_ABI *add_prepared)(void*,void*,float,float);
static void *ptr(void *p,unsigned n){uint32_t v;memcpy(&v,(char*)p+n,4);return (void*)(uintptr_t)v;}
static float number(void*p,unsigned n){float v;memcpy(&v,(char*)p+n,4);return v;}
static uintptr_t method(void *p,unsigned n,void **self){
 void *type=ptr(p,0),*vt=ptr(type,4);*self=(char*)p+*((unsigned char*)vt+n+4);return (uintptr_t)ptr(vt,n);
}
static int identity(void *m){
 if(!m)return 0;
 void *a=ptr(m,8);if(!a)return 0;
 uint32_t *shape=ptr(a,20);float *v=ptr(a,8);if(!shape||*shape!=16||!v)return 0;
 for(unsigned i=0;i<16;i++)if(v[i]!=(i%5==0?1.0f:0.0f))return 0;
 return 1;
}
void ANDROID_ABI text_fast(void *batch,void *font,void *text,float x,float y,float dx,float dy,float angle,int rounded,void *color){
 uint64_t profile_start=text_profile_enabled?scene_clock():0,profile_mark=0;
 unsigned reason=TF_ARGUMENT;
 if(!batch||!font||!text||!color)goto original;
 reason=TF_ANGLE;if(!isfinite(angle)||fabsf(angle)>360||(angle!=0&&!rotation_enabled))goto original;
 reason=TF_INIT;
 if(!text_initialized||!*text_initialized||!layout_slot||!*layout_slot||
    !restore_slot||!matrix_slot||!*matrix_slot||!vector_slot||!*vector_slot)goto original;
 reason=TF_COORD;
 if(!isfinite(x)||!isfinite(y)||!isfinite(dx)||!isfinite(dy)||fabsf(x)>100000||fabsf(y)>100000||fabsf(dx)>100000||fabsf(dy)>100000)goto original;
 void *self;uintptr_t set=method(batch,264,&self);
 /* Keep the original guards; count the first failed condition per call. */
 reason=TF_SETTER;if(set!=batch_setter||self!=batch)goto original;
 reason=TF_SHADER;if(ptr(batch,64))goto original;
 reason=TF_INACTIVE;if(!*((unsigned char*)batch+32))goto original;
 int integer_font=*((unsigned char*)font+21)!=0;
 reason=TF_INTEGER;if(integer_font&&!integer_enabled)goto original;
 int local_coordinates=angle!=0||integer_font;
 reason=TF_TRANSFORM;if(!identity(ptr(batch,36)))goto original;
 reason=TF_RESTORE;if(!identity(*restore_slot))goto original;
 reason=TF_DATA;
 void *(ANDROID_ABI *get_data)(void*)=(void*)method(font,248,&self);
 void *data=get_data(self);if(!data)goto original;
 if(rounded)y=(float)(int)round_double((double)y,1)+0.25f;
 if(number(data,60)==1.0f){x=(float)round_float(x);y=(float)round_float(y);dx=(float)round_float(dx);dy=(float)round_float(dy);}
 /* Preserve FontHelper's scratch objects, just as the original path does. */
 void *(ANDROID_ABI *rotation)(void*,float,float,float,float)=(void*)method(*matrix_slot,296,&self);
 rotation(self,0,0,1,angle);
 memcpy((char*)*vector_slot+8,&dx,4);memcpy((char*)*vector_slot+12,&dy,4);
 void *(ANDROID_ABI *rotate)(void*,float)=(void*)method(*vector_slot,472,&self);rotate(self,angle);
 float tx=x+number(*vector_slot,8),ty=y+number(*vector_slot,12);
 void *(ANDROID_ABI *translate)(void*,float,float,float)=(void*)method(*matrix_slot,120,&self);translate(self,tx,ty,0);
 void (ANDROID_ABI *set_color)(void*,void*)=(void*)method(font,88,&self);set_color(self,color);
 void *(ANDROID_ABI *draw)(void*,void*,void*,float,float)=(void*)method(font,40,&self);
 void *draw_self=self,*cache=ptr(font,16),*cache_self;
 int prepared=(uintptr_t)draw==font_draw_expected&&self==font&&cache&&method(cache,200,&cache_self)==cache_add_expected&&cache_self==cache;
 if(text_profile_enabled){profile_mark=scene_clock();text_setup_us+=profile_mark-profile_start;}
 void *layout=prepared?text_memo_get(font,data,text,color):NULL;
 if(text_profile_enabled){uint64_t now=scene_clock();text_memo_us+=now-profile_mark;profile_mark=now;}
 if(!layout){
  layout=*layout_slot;
  void (ANDROID_ABI *set_text)(void*,void*,void*)=(void*)method(layout,32,&self);set_text(self,font,text);
 }
 float width=number(layout,12),height=number(layout,16);
 float px=tx+width*-0.5f,py=ty+height*0.5f;
 if(local_coordinates){px=width*-0.5f;py=height*0.5f;}
 int transformed=0;
 if(prepared){
  void (ANDROID_ABI *clear)(void*)=(void*)method(cache,152,&cache_self);clear(cache_self);
  text_vertices_add(cache,layout,px,py,add_prepared);
  if(!local_coordinates||!text_submit_rotated_page(cache,batch,*matrix_slot)){
   if(local_coordinates){
    void (ANDROID_ABI *end)(void*)=(void*)method(batch,40,&self);end(self);
    ((void (ANDROID_ABI *)(void*,void*))batch_setter)(batch,*matrix_slot);
    void (ANDROID_ABI *begin)(void*)=(void*)method(batch,32,&self);begin(self);transformed=1;
   }
   void (ANDROID_ABI *draw_cache)(void*,void*)=(void*)method(cache,128,&cache_self);if(!text_submit_single_page(cache_self,batch))draw_cache(cache_self,batch);
  }
  prepared_count++;
 }else{
  if(local_coordinates){
   void (ANDROID_ABI *end)(void*)=(void*)method(batch,40,&self);end(self);
   ((void (ANDROID_ABI *)(void*,void*))batch_setter)(batch,*matrix_slot);
   void (ANDROID_ABI *begin)(void*)=(void*)method(batch,32,&self);begin(self);transformed=1;
  }
  draw(draw_self,batch,text,px,py);
 }
 if(transformed){
  void (ANDROID_ABI *end)(void*)=(void*)method(batch,40,&self);end(self);
  ((void (ANDROID_ABI *)(void*,void*))batch_setter)(batch,*restore_slot);
  void (ANDROID_ABI *begin)(void*)=(void*)method(batch,32,&self);begin(self);
 }
 if(angle!=0)rotation_count++;
 if(text_profile_enabled){uint64_t now=scene_clock();text_draw_us+=now-profile_mark;text_total_us+=now-profile_start;text_profile_calls++;}
 fast_count++;return;
 original:fallback_reason[reason]++;fallback_count++;text_original(batch,font,text,x,y,dx,dy,angle,rounded,color);
 if(text_profile_enabled){uint64_t now=scene_clock();text_fallback_us+=now-profile_start;text_total_us+=now-profile_start;text_profile_calls++;}
}
void text_fast_report(void){
 android_log_print(4,"TextRotation","enabled=%d calls=%u integer=%d",rotation_enabled,rotation_count,integer_enabled);rotation_count=0;
 FILE *control=fopen("ux0:data/spireandroid/text-rotation-command.txt","r");
 if(control){unsigned mode,serial;char extra;int n=fscanf(control,"text_rotation %u %u %c",&mode,&serial,&extra),failed=ferror(control);fclose(control);
  if(!remove("ux0:data/spireandroid/text-rotation-command.txt")&&!failed&&n==2&&mode<=2){text_rotation_enable(mode!=0);text_integer_enable(mode==2);android_log_print(4,"TextRotationChange","mode=%u serial=%u",mode,serial);}
 }
 if(text_profile_enabled)android_log_print(4,"TextCost","calls=%u total_us=%llu setup_us=%llu memo_us=%llu layout_draw_us=%llu fallback_us=%llu",text_profile_calls,(unsigned long long)text_total_us,(unsigned long long)text_setup_us,(unsigned long long)text_memo_us,(unsigned long long)text_draw_us,(unsigned long long)text_fallback_us);
 text_profile_calls=0;text_total_us=text_setup_us=text_memo_us=text_draw_us=text_fallback_us=0;
 android_log_print(4,"TextFast","fast=%u fallback=%u prepared=%u",fast_count,fallback_count,prepared_count);
 android_log_print(4,"TextFallback","argument=%u angle=%u init=%u coord=%u setter=%u shader=%u inactive=%u integer=%u transform=%u restore=%u data=%u",
 fallback_reason[0],fallback_reason[1],fallback_reason[2],fallback_reason[3],fallback_reason[4],fallback_reason[5],fallback_reason[6],fallback_reason[7],fallback_reason[8],fallback_reason[9],fallback_reason[10]);
 fast_count=fallback_count=prepared_count=0;memset(fallback_reason,0,sizeof(fallback_reason));
}
int text_fast_configure(ElfImage *im){
 const char *names[]={
 "_ZN3com8megacrit9cardcrawl7helpers32FontHelper$_$S_renderRotatedTextEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchEPNS6_10BitmapFontEPN6System6StringEfffffbPNS5_5ColorE",
 "_ZN3com8megacrit9cardcrawl7helpers10FontHelper6layoutE",
 "_ZN3com8megacrit9cardcrawl7helpers10FontHelper17rotatedTextMatrixE",
 "_ZN3com8megacrit9cardcrawl7helpers10FontHelper3mx4E",
 "_ZN3com8megacrit9cardcrawl7helpers10FontHelper14rotatedTextTmpE",
 "_ZZN6System8Internal12ClassHelpers9ClassCtorIN3com8megacrit9cardcrawl7helpers10FontHelperEEEvvE11_initalized",
 "_ZN3com8badlogic3gdx8graphics3g2d37SpriteBatch$_set_TransformMatrix_VoidEPNS3_11SpriteBatchEPNS1_4math7Matrix4E",
 "_ZN3com8badlogic3gdx4math19MathUtils$_$S_roundEf",
 "_ZN6System14Math$_$S_RoundEdNS_16MidpointRoundingE",
 "_ZN3com8badlogic3gdx8graphics3g2d28BitmapFont$_draw_GlyphLayoutEPNS3_10BitmapFontEPNS3_5BatchEPN6System6StringEff",
 "_ZN3com8badlogic3gdx8graphics3g2d36BitmapFontCache$_addText_GlyphLayoutEPNS3_15BitmapFontCacheEPN6System6StringEff",
 "_ZN3com8badlogic3gdx8graphics3g2d29BitmapFontCache$_addText_VoidEPNS3_15BitmapFontCacheEPNS3_11GlyphLayoutEff"};
 uint32_t v[12],size;if(!im||!im->relocated)return -1;
 for(unsigned i=0;i<12;i++)if(elf_image_symbol(im,names[i],v+i,&size)||!elf_image_contains(im,v[i]&~1u,i==5?1:4,(i==0||i>=6)?5:6))return -1;
 const unsigned char signature[]={0x2d,0xe9,0xf0,0x47,0x2d,0xed,0x0a,0x8b};
 if((v[0]&3)!=1||memcmp(im->base+(v[0]&~1u),signature,8))return -1;
 layout_slot=(void*)(im->base+v[1]);restore_slot=(void*)(im->base+v[2]);matrix_slot=(void*)(im->base+v[3]);vector_slot=(void*)(im->base+v[4]);text_initialized=im->base+v[5];
 batch_setter=(uintptr_t)(im->base+v[6]);round_float=(void*)(im->base+v[7]);round_double=(void*)(im->base+v[8]);
 font_draw_expected=(uintptr_t)(im->base+v[9]);cache_add_expected=(uintptr_t)(im->base+v[10]);add_prepared=(void*)(im->base+v[11]);
 text_original_resume=(uint32_t)(uintptr_t)(im->base+(v[0]&~1u)+8)|1;
 unsigned char *at=im->base+(v[0]&~1u);const unsigned char jump[]={0xdf,0xf8,0,0xf0};uint32_t target=(uint32_t)(uintptr_t)text_fast;
 memcpy(at,jump,4);memcpy(at+4,&target,4);return 0;
}
