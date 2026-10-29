#include "load_profile.h"
#include "direct_cache.h"
#include "launcher_background.h"
static int libraries_ready,preferences_reloaded;
#include "scene_metrics.h"
#include "android_log.h"
#include <string.h>
/* Inclusive CPU/wait time of original managed calls. No work is skipped.
 * PLT hooks only: direct/vtable calls may not be observed. */
static void *original[10];
static void report(unsigned i, const char *name, uint64_t start){
 uint64_t end=scene_clock();
 /* XB1Init is also polled after completion; avoid per-frame logging. */
 if(i==5 && end-start<10000)return;
 android_log_print(4,"LoadProfile","name=%s begin_us=%llu work_us=%llu",name,(unsigned long long)start,(unsigned long long)(end-start));
}
static void hook_0(void *a){uint64_t start=scene_clock();((void (*)(void *))original[0])(a);report(0,"menu_buttons",start);}
static void hook_1(void){uint64_t start=scene_clock();launcher_background_stage(7,"Loading profile");((void (*)(void))original[1])();preferences_reloaded=1;report(1,"reload_profile_libraries",start);}
static void hook_2(void){uint64_t start=scene_clock();direct_cache_loading_begin();((void (*)(void))original[2])();direct_cache_loading_end();libraries_ready=1;report(2,"initial_profile_libraries",start);}
static void hook_3(void){uint64_t start=scene_clock();launcher_background_stage(4,"Loading cards");((void (*)(void))original[3])();report(3,"card_library",start);}
static void hook_4(void){uint64_t start=scene_clock();launcher_background_stage(5,"Loading relics");((void (*)(void))original[4])();report(4,"relic_library",start);}
static void hook_5(void *a){uint64_t start=scene_clock();((void (*)(void *a))original[5])(a);if(libraries_ready&&preferences_reloaded){launcher_background_stage(8,"Preparing main menu");launcher_background_ready();}report(5,"menu_profile_setup",start);}
static void * hook_6(void *a){uint64_t start=scene_clock();launcher_background_stage(6,"Loading characters");void *result=((void * (*)(void *a))original[6])(a);report(6,"character_manager",start);return result;}
static void * hook_7(void *a){uint64_t start=scene_clock();void *result=((void * (*)(void *))original[7])(a);report(7,"options_panel",start);return result;}
static void * hook_8(void *a){uint64_t start=scene_clock();void *result=((void * (*)(void *a))original[8])(a);report(8,"read_save",start);return result;}
static void * hook_9(void *a){uint64_t start=scene_clock();void *result=((void * (*)(void *))original[9])(a);report(9,"input_settings",start);return result;}
static const struct {const char *name;uint32_t value,size;unsigned char signature[8];void *hook;} targets[]={
{"_ZN3com8megacrit9cardcrawl7screens8mainMenu34MainMenuScreen$_setMainMenuButtonsEPNS3_14MainMenuScreenE",0x213b9bd,768,{0xf0,0xb5,0x81,0xb0,0x4,0x46,0xb2,0x48},(void*)hook_0},
{"_ZN3com8megacrit9cardcrawl4core35CardCrawlGame$_$S_reloadPrefs_CloudEv",0x1efaa01,176,{0x10,0xb5,0x22,0x48,0x78,0x44,0x0,0x68},(void*)hook_1},
{"_ZN3com8megacrit9cardcrawl4core27CardCrawlGame$_$S_CloudInitEv",0x1f02835,188,{0x24,0x48,0x78,0x44,0x1,0x68,0x8,0x78},(void*)hook_2},
{"_ZN3com8megacrit9cardcrawl7helpers26CardLibrary$_$S_initializeEv",0x1f8bf9d,1660,{0x2d,0xe9,0xf0,0x4f,0x89,0xb0,0xf9,0x48},(void*)hook_3},
{"_ZN3com8megacrit9cardcrawl7helpers27RelicLibrary$_$S_initializeEv",0x1fc798d,2076,{0x2d,0xe9,0xf0,0x41,0x82,0xb0,0x7,0x48},(void*)hook_4},
{"_ZN3com8megacrit9cardcrawl7screens8mainMenu23MainMenuScreen$_XB1InitEPNS3_14MainMenuScreenE",0x213be11,368,{0xb0,0xb5,0x4,0x46,0x4c,0x48,0x78,0x44},(void*)hook_5},
{"_ZN3com8megacrit9cardcrawl10characters23CharacterManager$_$CtorEPNS2_16CharacterManagerE",0x1eea389,568,{0xf0,0xb5,0x85,0xb0,0x4,0x46,0x78,0xb9},(void*)hook_6},
{"_ZN3com8megacrit9cardcrawl7screens7options19OptionsPanel$_$CtorEPNS3_12OptionsPanelE",0x215d301,2788,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed},(void*)hook_7},
{"_ZN3com8megacrit9cardcrawl15saveAndContinue32SaveAndContinue$_$S_loadSaveFileEPN6System6StringE",0x20decd1,440,{0x2d,0xe9,0xf0,0x47,0x81,0x46,0x60,0x48},(void*)hook_8},
{"_ZN3com8megacrit9cardcrawl7screens7options26InputSettingsScreen$_$CtorEPNS3_19InputSettingsScreenE",0x2159531,888,{0xf0,0xb5,0x81,0xb0,0x2d,0xed,0x2,0x8b},(void*)hook_9},
};
int load_profile_configure(ElfImage *im){
 uint32_t slots[10];
 if(!im||!im->relocated)return -1;
 for(unsigned i=0;i<10;i++){
  uint32_t v,s,current;
  if(elf_image_symbol(im,targets[i].name,&v,&s)||v!=targets[i].value||s!=targets[i].size||
     !elf_image_contains(im,v&~1u,8,5)||memcmp(im->base+(v&~1u),targets[i].signature,8)||
     elf_image_relocation_target(im,targets[i].name,22,&slots[i])||(slots[i]&3)||!elf_image_contains(im,slots[i],4,2))return -1;
  memcpy(&current,im->base+slots[i],4);
  if(current!=(uint32_t)(uintptr_t)(im->base+v))return -1;
 }
 for(unsigned i=0;i<10;i++){
  original[i]=im->base+targets[i].value;
  uint32_t target=(uint32_t)(uintptr_t)targets[i].hook;
  memcpy(im->base+slots[i],&target,4);
 }
 return 0;
}
