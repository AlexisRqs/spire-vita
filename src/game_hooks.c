#include "game_hooks.h"
#include "launcher_settings.h"
#include "lazy_character.h"
#include "scene_metrics.h"
#include "android_log.h"
#include <string.h>
#include <stdio.h>
uint32_t game_resumes[5],game_character_literal;
extern void game_original_create(void*);
extern void game_original_room(void*,void*);
extern void game_original_character(void*,int);
extern void game_original_map(void*,int);
extern void game_original_reward(void*,void*,void*,void*);
static void *(*make_string)(const char *);
static void *(*load_image)(void *);
static void (*add_roots)(void *,void *);
static void *glyphs[24];
static unsigned char attempted[24];
static int rooted;
static const int keys[]={0,1,2,3,4,5,6,7,8,9,-2000,2000,-2001,2001,-1000,1000,-1001,1001,-1002,1002,-1003,1003,1004,1005};
static const char *files[]={"a","b","x","y","lb","rb","back","start","ls","rs","up","down","left","right","ls_up","ls_down","ls_left","ls_right","rs_up","rs_down","rs_left","rs_right","lt","rt"};
void *game_glyph(void *action){
 if(!action)return NULL;
 int key;memcpy(&key,(unsigned char*)action+8,4);
 unsigned i;for(i=0;i<24;i++)if(keys[i]==key)break;
 if(i==24)i=0; /* Same fallback as original get_KeyImg. */
 if(!attempted[i]){
  attempted[i]=1;
  if(!rooted){add_roots(glyphs,glyphs+24);rooted=1;}
  char path[96];snprintf(path,sizeof(path),"images/ui/controller/ps4/%s.png",files[i]);
  unsigned t=scene_begin("controller_glyph");
  void *name=make_string(path);glyphs[i]=name?load_image(name):NULL;
  scene_return(t);
  android_log_print(glyphs[i]?4:5,"ControllerGlyph","key=%d path=%s loaded=%u",key,path,glyphs[i]!=NULL);
 }
 return glyphs[i];
}
static unsigned char *disable_effects;
static void (*create_game)(void*);
static void *(*menu_create)(void*,int);
static void (*room_change)(void*,void*);
static void (*character_open)(void*,int);
static void (*map_open)(void*,int);
static void (*reward_open)(void*,void*,void*,void*);
static void hook_create(void *a){unsigned t=scene_begin("startup");create_game(a);scene_return(t);}
static unsigned char original_effects;
static int effects_captured;
void game_settings_apply_launcher(void){
 if(!disable_effects||!effects_captured)return;
 *disable_effects=launcher_settings.effects?launcher_settings.effects==1:original_effects;
}
static void *hook_menu(void *a,int b){
 unsigned t=scene_begin("main_menu");void *r=menu_create(a,b);scene_return(t);
 /* Game initialization has now read its preferences. Apply only the explicit
  * launcher override; never rewrite the user's gameplay preference file. */
 if(disable_effects&&!effects_captured){original_effects=*disable_effects;effects_captured=1;}
 game_settings_apply_launcher();
 if(disable_effects)android_log_print(4,"GameSettings","disable_effects=%u",*disable_effects);
 return r;
}
static void hook_room(void *a,void *b){unsigned t=scene_begin("room_transition");room_change(a,b);scene_return(t);}
static void hook_character(void *a,int b){unsigned t=scene_begin("character_select");lazy_character_prepare(a);character_open(a,b);scene_return(t);}
static void hook_map(void *a,int b){unsigned t=scene_begin("dungeon_map");map_open(a,b);scene_return(t);}
static void hook_reward(void *a,void *b,void*c,void*d){unsigned t=scene_begin("card_reward");reward_open(a,b,c,d);scene_return(t);}
static const char *targets[]={
 "_ZN3com8megacrit9cardcrawl7helpers10controller32CInputAction$_get_KeyImg_TextureEPNS3_12CInputActionE",
 "_ZN3com8megacrit9cardcrawl4core26CardCrawlGame$_create_VoidEPNS2_13CardCrawlGameE",
 "_ZN3com8megacrit9cardcrawl7screens8mainMenu21MainMenuScreen$_$CtorEPNS3_14MainMenuScreenEb",
 "_ZN3com8megacrit9cardcrawl8dungeons40AbstractDungeon$_nextRoomTransition_VoidEPNS2_15AbstractDungeonEPNS1_15saveAndContinue8SaveFileE",
 "_ZN3com8megacrit9cardcrawl7screens10charSelect32CharacterSelectScreen$_open_VoidEPNS3_21CharacterSelectScreenEb",
 "_ZN3com8megacrit9cardcrawl7screens27DungeonMapScreen$_open_VoidEPNS2_16DungeonMapScreenEb",
 "_ZN3com8megacrit9cardcrawl7screens27CardRewardScreen$_open_VoidEPNS2_16CardRewardScreenEPN6System11Collections7Generic6List$1IPNS1_5cards12AbstractCardEEEPNS1_7rewards10RewardItemEPNS5_6StringE"};
int game_hooks_configure(ElfImage *im){
 if(!im||!im->relocated)return -1;
 uint32_t effect_value,effect_size;
 if(!elf_image_symbol(im,"_ZN3com8megacrit9cardcrawl4core8Settings15DISABLE_EFFECTSE",&effect_value,&effect_size)&&effect_size==1&&elf_image_contains(im,effect_value,1,6))disable_effects=im->base+effect_value;
 static const unsigned lengths[]={8,10,0,8,8,8,10};
 static const unsigned char signatures[7][10]={{0x10,0xb5,0x80,0x68,0x10,0xf5,0x7a,0x7f},{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x02,0x8b},{0},{0x2d,0xe9,0xf0,0x4f,0x93,0xb0,0x82,0x46},{0x2d,0xe9,0xf0,0x41,0x80,0x46,0x55,0x48},{0x70,0xb5,0x2d,0xed,0x02,0x8b,0x04,0x46},{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x02,0x8b}};
 uint32_t values[7],helpers[3],menu_slot=0;
 const char *helper_names[]={"_ZN6System7Runtime15InteropServices27Marshal$_$S_PtrToStringAnsiENS_6IntPtrE","_ZN3com8megacrit9cardcrawl7helpers25ImageMaster$_$S_loadImageEPN6System6StringE","GC_add_roots"};
 /* APK 2.6.1 only. Check every entry before modifying executable bytes.
  * Non-PLT methods are called directly / through managed vtables. */
 for(unsigned i=0;i<7;i++){uint32_t size;
  if(elf_image_symbol(im,targets[i],&values[i],&size)||!elf_image_contains(im,values[i]&~1u,lengths[i]?lengths[i]:2,5))return -1;
  if(lengths[i]&&((values[i]&2)||memcmp(im->base+(values[i]&~1u),signatures[i],lengths[i])))return -1;
 }
 uint32_t current;
 if(elf_image_relocation_target(im,targets[2],22,&menu_slot)||(menu_slot&3)||!elf_image_contains(im,menu_slot,4,2))return -1;
 memcpy(&current,im->base+menu_slot,4);if(current!=(uint32_t)(uintptr_t)(im->base+values[2]))return -1;
 for(unsigned i=0;i<3;i++){uint32_t size;if(elf_image_symbol(im,helper_names[i],&helpers[i],&size)||!elf_image_contains(im,helpers[i]&~1u,2,5))return -1;}
 if(!elf_image_contains(im,(values[4]&~1u)+0x15c,4,4))return -1;
 memcpy(&game_character_literal,im->base+(values[4]&~1u)+0x15c,4);
 unsigned indices[]={1,3,4,5,6};
 for(unsigned i=0;i<5;i++){unsigned n=indices[i];game_resumes[i]=(uint32_t)(uintptr_t)(im->base+(values[n]&~1u)+lengths[n])|1;}
 create_game=game_original_create;menu_create=(void*)(im->base+values[2]);room_change=game_original_room;character_open=game_original_character;map_open=game_original_map;reward_open=game_original_reward;
 make_string=(void*)(im->base+helpers[0]);load_image=(void*)(im->base+helpers[1]);add_roots=(void*)(im->base+helpers[2]);
 uintptr_t hooks[]={(uintptr_t)game_glyph,(uintptr_t)hook_create,(uintptr_t)hook_menu,(uintptr_t)hook_room,(uintptr_t)hook_character,(uintptr_t)hook_map,(uintptr_t)hook_reward};
 for(unsigned i=0;i<7;i++){
  uint32_t target=(uint32_t)hooks[i];
  if(i==2)memcpy(im->base+menu_slot,&target,4);
  else{unsigned char *at=im->base+(values[i]&~1u);const unsigned char branch[]={0xdf,0xf8,0,0xf0};memcpy(at,branch,4);memcpy(at+4,&target,4);}
 }
 return 0;
}
