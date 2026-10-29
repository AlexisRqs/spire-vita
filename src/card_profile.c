#include "card_profile.h"
#include "grid_card_observer.h"
#include "compat_libc.h"
#include "scene_metrics.h"
#include "android_log.h"
#include <string.h>
#include <stdatomic.h>
/* Inclusive PLT-call durations; nested counters MUST NOT be summed.
 * Direct and virtual calls bypassing PLT are not observed. No game work skipped.
 * Atomic counters also accept calls from background threads. */
#define N 10
static void *original[N];
static atomic_uint elapsed[N],calls[N];
static int enabled;
static void record(unsigned i,uint64_t start){atomic_fetch_add(&elapsed[i],(unsigned)(scene_clock()-start));atomic_fetch_add(&calls[i],1);}
static void ANDROID_ABI hook_0(void *a,void *b){grid_card_observer_visit(a);uint64_t t=scene_clock();((void (ANDROID_ABI *)(void *a,void *b))original[0])(a,b);record(0,t);}
static void ANDROID_ABI hook_1(void *a,void *b){uint64_t t=scene_clock();((void (ANDROID_ABI *)(void *a,void *b))original[1])(a,b);record(1,t);}
static void ANDROID_ABI hook_2(void *a,void *b){uint64_t t=scene_clock();((void (ANDROID_ABI *)(void *a,void *b))original[2])(a,b);record(2,t);}
static void ANDROID_ABI hook_3(void *a,void *b){uint64_t t=scene_clock();((void (ANDROID_ABI *)(void *a,void *b))original[3])(a,b);record(3,t);}
static void ANDROID_ABI hook_4(void *a,void *b){uint64_t t=scene_clock();((void (ANDROID_ABI *)(void *a,void *b))original[4])(a,b);record(4,t);}
static void ANDROID_ABI hook_5(void *a,int b,void *c,int d,int e){uint64_t t=scene_clock();((void (ANDROID_ABI *)(void *a,int b,void *c,int d,int e))original[5])(a,b,c,d,e);record(5,t);}
static void ANDROID_ABI hook_6(void *a,void *b){uint64_t t=scene_clock();((void (ANDROID_ABI *)(void*,void*))original[6])(a,b);record(6,t);}
static void ANDROID_ABI hook_7(void *a,void *b,int c,int d){uint64_t t=scene_clock();((void (ANDROID_ABI *)(void*,void*,int,int))original[7])(a,b,c,d);record(7,t);}
static void ANDROID_ABI hook_8(void *a,void *b,int c,int d){uint64_t t=scene_clock();((void (ANDROID_ABI *)(void*,void*,int,int))original[8])(a,b,c,d);record(8,t);}
static void ANDROID_ABI hook_9(void *a,void *b){uint64_t t=scene_clock();((void (ANDROID_ABI *)(void*,void*))original[9])(a,b);record(9,t);}
static const struct {const char *name,*label;uint32_t value,size;unsigned char signature[8];void *hook;} targets[N]={
{"_ZN3com8megacrit9cardcrawl5cards31AbstractCard$_renderDescriptionEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","description",0x1e6f92d,5492,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed},(void*)hook_0},
{"_ZN3com8megacrit9cardcrawl5cards25AbstractCard$_renderTitleEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","title",0x1e6e551,1164,{0xf0,0xb5,0x87,0xb0,0x5,0x46,0xeb,0x48},(void*)hook_1},
{"_ZN3com8megacrit9cardcrawl5cards28AbstractCard$_renderPortraitEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","portrait",0x1e72411,808,{0x2d,0xe9,0xf0,0x41,0x2d,0xed,0x8,0x8b},(void*)hook_2},
{"_ZN3com8megacrit9cardcrawl5cards24AbstractCard$_renderTypeEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","type",0x1e6e1fd,852,{0xf0,0xb5,0x87,0xb0,0x5,0x46,0xc9,0x48},(void*)hook_3},
{"_ZN3com8megacrit9cardcrawl5cards26AbstractCard$_renderEnergyEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","energy",0x1e70f59,924,{0x2d,0xe9,0xf0,0x41,0x86,0xb0,0x5,0x46},(void*)hook_4},
{"_ZN6System14Array$_$S_CopyEPNS_5ArrayEiS1_ii","array_copy",0x1b98c81,1280,{0x2d,0xe9,0xf0,0x4f,0x87,0xb0,0x5,0x46},(void*)hook_5},
{"_ZN3com8megacrit9cardcrawl5cards24AbstractCard$_renderGlowEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","glow",0x1e6dddd,312,{0x70,0xb5,0x84,0xb0,0x5,0x46,0x46,0x48},(void*)hook_6},
{"_ZN3com8megacrit9cardcrawl5cards24AbstractCard$_renderCardEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchEbb","card",0x1e713a9,420,{0xf0,0xb5,0x83,0xb0,0x5,0x46,0x5b,0x48},(void*)hook_7},
{"_ZN3com8megacrit9cardcrawl5cards25AbstractCard$_renderImageEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchEbb","image",0x1e6df15,744,{0x2d,0xe9,0xf0,0x43,0x81,0xb0,0x2d,0xed},(void*)hook_8},
{"_ZN3com8megacrit9cardcrawl5cards30AbstractCard$_renderMainBorderEPNS2_12AbstractCardEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","border",0x1e71969,528,{0x70,0xb5,0x88,0xb0,0x5,0x46,0x75,0x48},(void*)hook_9}
};
void card_profile_report(void){if(!enabled)return;for(unsigned i=0;i<N;i++){unsigned us=atomic_exchange(&elapsed[i],0),count=atomic_exchange(&calls[i],0);android_log_print(4,"CardCPU","name=%s calls=%u inclusive_us=%u",targets[i].label,count,us);}}
int card_profile_configure(ElfImage *im){
 uint32_t slots[N];if(!im||!im->relocated||enabled)return -1;
 for(unsigned i=0;i<N;i++){uint32_t v,s,current;
  if(elf_image_symbol(im,targets[i].name,&v,&s)||v!=targets[i].value||s!=targets[i].size||!elf_image_contains(im,v&~1u,8,5)||memcmp(im->base+(v&~1u),targets[i].signature,8)||elf_image_relocation_target(im,targets[i].name,22,&slots[i])||(slots[i]&3)||!elf_image_contains(im,slots[i],4,2))return -1;
  memcpy(&current,im->base+slots[i],4);if(current!=(uint32_t)(uintptr_t)(im->base+v))return -1;
 }
 for(unsigned i=0;i<N;i++){original[i]=im->base+targets[i].value;uint32_t target=(uint32_t)(uintptr_t)targets[i].hook;memcpy(im->base+slots[i],&target,4);atomic_store(&elapsed[i],0);atomic_store(&calls[i],0);}enabled=1;return 0;
}
