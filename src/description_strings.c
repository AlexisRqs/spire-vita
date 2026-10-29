#include "description_strings.h"
#include "compat_libc.h"
#include "android_log.h"
#include <string.h>
#include <stdatomic.h>
#define DS_SLOTS 128
#define DS_CHARS 96
/* Only four audited callsites inside renderDescription may share immutable
 * results. Other users retain original allocation/reference semantics. */
static void *(ANDROID_ABI *sub1)(void*,int);
static void *(ANDROID_ABI *sub2)(void*,int,int);
static void *(ANDROID_ABI *concat2)(void*,void*);
static void (ANDROID_ABI *roots)(void*,void*);
static uintptr_t image_base;
static uint32_t *gc_stop;
static atomic_flag busy=ATOMIC_FLAG_INIT;
static void *results[DS_SLOTS]; /* registered explicitly with Android GC */
typedef struct {uint16_t chars[DS_CHARS];unsigned length,age;} Entry;
static Entry entries[DS_SLOTS];
static unsigned clock_age,validation_serial,rooted,configured;
static atomic_uint hits,misses,bypass,checks,mismatch,disabled;
static unsigned length(void *s){unsigned n;memcpy(&n,(char*)s+8,4);return n;}
static int matches(void *s,const uint16_t *chars,unsigned n){return s&&length(s)==n&&!memcmp((char*)s+12,chars,n*2);}
static int allowed(unsigned op,uintptr_t caller){
 caller&=~(uintptr_t)1;
 return op==0?caller==image_base+0x1e6fcc6:op==1?caller==image_base+0x1e6fd3a:(caller==image_base+0x1e6fd28||caller==image_base+0x1e6fd4a);
}
static void *original(unsigned op,void *a,void *b,int start,int n){return op==0?sub1(a,start):op==1?sub2(a,start,n):concat2(a,b);}
static void *lookup(unsigned op,void *a,void *b,int start,int count,uintptr_t caller){
 uint16_t chars[DS_CHARS];unsigned n,na,nb=0;
 if(!allowed(op,caller)||disabled||!gc_stop||*gc_stop||!a)goto fallback;
 na=length(a);if(!na||na>DS_CHARS)goto fallback;
 if(op==2){
  if(!b||(nb=length(b))==0||nb>DS_CHARS||na+nb>DS_CHARS)goto fallback;
  n=na+nb;memcpy(chars,(char*)a+12,na*2);memcpy(chars+na,(char*)b+12,nb*2);
 }else{
  if(start<0||(unsigned)start>na)goto fallback;
  if(op==0)count=(int)na-start;
  if(count<=0||(unsigned)count>na-(unsigned)start||(!start&&(unsigned)count==na))goto fallback;
  n=(unsigned)count;memcpy(chars,(char*)a+12+start*2,n*2);
 }
 if(atomic_flag_test_and_set(&busy))goto fallback;
 /* Invalid arguments never enter the cache. An unexpected managed exception
  * leaves the flag set: future calls safely use original, never spin. */
 if(!rooted){roots(results,results+DS_SLOTS);rooted=1;}
 uint32_t hash=2166136261u;for(unsigned i=0;i<n;i++)hash=(hash^chars[i])*16777619u;
 unsigned base=(hash%(DS_SLOTS/4))*4,victim=base;void *r;
 for(unsigned i=base;i<base+4;i++){
  if(results[i]&&entries[i].length==n&&!memcmp(entries[i].chars,chars,n*2)){
   if(!matches(results[i],chars,n)){disabled=1;mismatch++;goto fresh;}
   hits++;entries[i].age=++clock_age;
   if((++validation_serial&255)==0){checks++;r=original(op,a,b,start,count);if(!matches(r,chars,n)){disabled=1;mismatch++;atomic_flag_clear(&busy);return r;}}
   r=results[i];atomic_flag_clear(&busy);return r;
  }
  if(!results[i]||entries[i].age<entries[victim].age)victim=i;
 }
 fresh:
 misses++;r=original(op,a,b,start,count);
 if(!disabled&&matches(r,chars,n)){
  results[victim]=r;entries[victim].length=n;entries[victim].age=++clock_age;memcpy(entries[victim].chars,chars,n*2);
 }else if(!disabled){disabled=1;mismatch++;}
 atomic_flag_clear(&busy);return r;
 fallback:bypass++;return original(op,a,b,start,count);
}
static __attribute__((noinline)) void *ANDROID_ABI hook_sub1(void *a,int s){return lookup(0,a,0,s,0,(uintptr_t)__builtin_return_address(0));}
static __attribute__((noinline)) void *ANDROID_ABI hook_sub2(void *a,int s,int n){return lookup(1,a,0,s,n,(uintptr_t)__builtin_return_address(0));}
static __attribute__((noinline)) void *ANDROID_ABI hook_concat(void *a,void *b){return lookup(2,a,b,0,0,(uintptr_t)__builtin_return_address(0));}
static const struct {const char *name;unsigned value,size;unsigned char signature[8];void *hook;} targets[]={
 {"_ZN6System17String$_SubstringEPNS_6StringEi",0x1c23f15,108,{0xb0,0xb5,0x0c,0x46,0x16,0x49,0x79,0x44},(void*)hook_sub1},
 {"_ZN6System17String$_SubstringEPNS_6StringEii",0x1c23f81,300,{0x70,0xb5,0x40,0x4b,0x7b,0x44,0x1b,0x68},(void*)hook_sub2},
 {"_ZN6System17String$_$S_ConcatEPNS_6StringES1_",0x1c285fd,240,{0xf0,0xb5,0x81,0xb0,0x05,0x46,0x37,0x48},(void*)hook_concat}
};
int description_strings_configure(ElfImage *im){
 if(!im||!im->relocated||configured)return -1;
 uint32_t slots[3],v,s,root,stop;
 for(unsigned i=0;i<3;i++){
  if(elf_image_symbol(im,targets[i].name,&v,&s)||v!=targets[i].value||s!=targets[i].size||!elf_image_contains(im,v&~1u,8,5)||memcmp(im->base+(v&~1u),targets[i].signature,8)||elf_image_relocation_target(im,targets[i].name,22,&slots[i])||(slots[i]&3)||!elf_image_contains(im,slots[i],4,2))return -1;
  uint32_t current;memcpy(&current,im->base+slots[i],4);if(current!=(uint32_t)(uintptr_t)(im->base+v))return -1;
 }
 if(elf_image_symbol(im,"GC_add_roots",&root,&s)||!elf_image_contains(im,root&~1u,4,5)||elf_image_symbol(im,"__$gc_stop_threads",&stop,&s)||!elf_image_contains(im,stop,4,6))return -1;
 /* Confirm the caller method identity, not just a borrowed address range. */
 if(elf_image_symbol(im,"_ZN3com8megacrit9cardcrawl5cards31AbstractCard$_renderDescriptionEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE",&v,&s)||v!=0x1e6f92d||s!=5492)return -1;
 image_base=(uintptr_t)im->base;gc_stop=(void*)(im->base+stop);roots=(void*)(im->base+root);
 sub1=(void*)(im->base+targets[0].value);sub2=(void*)(im->base+targets[1].value);concat2=(void*)(im->base+targets[2].value);
 for(unsigned i=0;i<3;i++){uint32_t h=(uint32_t)(uintptr_t)targets[i].hook;memcpy(im->base+slots[i],&h,4);}configured=1;return 0;
}
void description_strings_report(void){if(!configured)return;android_log_print(4,"DescriptionStrings","hits=%u misses=%u bypass=%u checks=%u mismatch=%u disabled=%u",hits,misses,bypass,checks,mismatch,disabled);hits=misses=bypass=checks=mismatch=0;}
