#include "lazy_character.h"
#include "scene_metrics.h"
#include "android_log.h"
#include <string.h>
static void (*initialize)(void *);
void lazy_character_prepare(void *screen){
 if(!initialize||!screen)return;
 /* options is a managed List at +20, whose count is at +12. The list belongs
  * to the screen: no native pointer cache or extra GC roots are needed. */
 void *options;memcpy(&options,(unsigned char*)screen+20,sizeof options);
 int count=0;if(options)memcpy(&count,(unsigned char*)options+12,4);
 if(count)return;
 unsigned token=scene_begin("character_prepare");initialize(screen);scene_return(token);
 android_log_print(4,"LazyCharacter","Selection initialized before open");
}
int lazy_character_configure(ElfImage *im){
 const char *menu="_ZN3com8megacrit9cardcrawl7screens8mainMenu23MainMenuScreen$_XB1InitEPNS3_14MainMenuScreenE";
 const char *init="_ZN3com8megacrit9cardcrawl7screens10charSelect38CharacterSelectScreen$_initialize_VoidEPNS3_21CharacterSelectScreenE";
 const unsigned char call[]={0xa0,0x6f,0x68,0xb1,0x01,0x68,0x49,0x68,0x91,0xf8,0x24,0x20,0x09,0x6a,0x10,0x44,0x88,0x47};
 const unsigned char entry[]={0x2d,0xe9,0xf0,0x47,0x2d,0xed,0x08,0x8b};
 uint32_t m,ms,i,is;
 if(!im||!im->relocated||elf_image_symbol(im,menu,&m,&ms)||m!=0x213be11||ms!=368||
    elf_image_symbol(im,init,&i,&is)||i!=0x20f9859||is!=2284||
    !elf_image_contains(im,0x213befe,sizeof call,5)||!elf_image_contains(im,i&~1u,8,5)||
    memcmp(im->base+0x213befe,call,sizeof call)||memcmp(im->base+(i&~1u),entry,8))return -1;
 /* Skip only this eager call. The existing open hook invokes the original
  * initializer before any original open logic can access character options.
  * UnlockTracker.refresh and every other setup step remain intact. */
 initialize=(void*)(im->base+i);
 const unsigned char nop[]={0x00,0xbf};memcpy(im->base+0x213bf0e,nop,2);
 return 0;
}
