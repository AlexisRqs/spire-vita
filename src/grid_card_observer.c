#include "grid_card_observer.h"
#include "android_log.h"
#include <string.h>
#define CARD_LIMIT 64u
#define CARD_BYTES 1024u
static const struct {const char *name;uint32_t address;} card_types[]={
#include "grid_card_types.inc"
};
#define TYPE_COUNT (sizeof card_types/sizeof card_types[0])
static void **cached_types[TYPE_COUNT];
static uint32_t *stop;
static void **glow_list_type;
static unsigned char *tip_rendered;
typedef struct {void *object;uint32_t header,size;unsigned char after[CARD_BYTES],before[CARD_BYTES];} Card;
static Card cards[CARD_LIMIT];
static int quiet;
static unsigned count,active,pass,frame,first_frame,valid,tip_before,visits;
static uint32_t readword(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static int available(void){return stop&&!*stop&&tip_rendered;}
static unsigned object_size(void *object){
 if(!available()||!object)return 0;
 uint32_t header=readword(object);
 for(unsigned i=0;i<TYPE_COUNT;i++)if(cached_types[i]&&*cached_types[i]&&header==(uint32_t)((uintptr_t)*cached_types[i]+0x38)){
  unsigned size=readword(*cached_types[i]);return size>=4&&size<=CARD_BYTES?size:0;
 }
 return 0;
}
void grid_card_observer_begin(unsigned ordinal,unsigned current,int sample){
 active=0;
 if(!sample||ordinal<1||ordinal>2||!available())return;
 pass=ordinal;frame=current;active=1;visits=0;quiet=sample>1;tip_before=*tip_rendered;
 if(pass==1){count=0;valid=1;first_frame=frame;return;}
 if(first_frame!=frame){valid=0;return;}
 for(unsigned i=0;i<count;i++){
  Card *c=cards+i;
  if(object_size(c->object)!=c->size||readword(c->object)!=c->header){valid=0;continue;}
  memcpy(c->before,c->object,c->size);
  if(memcmp(c->after,c->before,c->size)){valid=0;if(!quiet)android_log_print(4,"GridCardState","frame=%u card=%u between_changed=1",frame,i);}
 }
}
void grid_card_observer_visit(void *card){
 if(!active)return;
 visits++;
 for(unsigned i=0;i<count;i++)if(cards[i].object==card)return;
 if(pass!=1||count==CARD_LIMIT){valid=0;return;}
 unsigned size=object_size(card);if(!size){valid=0;return;}
 Card *c=cards+count++;c->object=card;c->size=size;c->header=readword(card);
}
void grid_card_observer_end(void){
 if(!active)return;
 active=0;
 if(!available())valid=0;
 unsigned changed=0;
 if(valid)for(unsigned i=0;i<count;i++){
  Card *c=cards+i;
  if(object_size(c->object)!=c->size||readword(c->object)!=c->header){valid=0;break;}
  if(pass==1)memcpy(c->after,c->object,c->size);
  else for(unsigned at=0;at<c->size;at+=4){
   unsigned n=c->size-at<4?c->size-at:4;
   if(memcmp(c->before+at,(unsigned char*)c->object+at,n)){
    if(!quiet&&changed<32)android_log_print(4,"GridCardField","frame=%u card=%u offset=%u bytes=%u",frame,i,at,n);
    changed++;
   }
  }
 }
 if(!quiet)android_log_print(4,"GridCards","frame=%u pass=%u valid=%u count=%u visits=%u second_changed_words=%u tip_before=%u tip_after=%u description_observed_only=1",frame,pass,valid,count,visits,changed,tip_before,available()?*tip_rendered:2);
}
static int glow_inactive(const unsigned char *card,unsigned size,unsigned list_count){return size>0xf8&&!card[0xf8]&&!list_count;}
int grid_card_observer_can_reuse(unsigned current){
 if(!available()||!active||pass!=2||frame!=current||first_frame!=current||!valid||!count||!*tip_rendered||!glow_list_type||!*glow_list_type)return 0;
 unsigned list_size=readword(*glow_list_type);if(list_size<16||list_size>CARD_BYTES)return 0;
 for(unsigned i=0;i<count;i++){
  Card *c=cards+i;
  if(c->size<=0xf8||object_size(c->object)!=c->size||memcmp(c->after,c->object,c->size))return 0;
  void *list=(void*)(uintptr_t)readword(c->after+0xf0);
  if(!list||readword(list)!=(uint32_t)((uintptr_t)*glow_list_type+0x38)||!glow_inactive(c->after,c->size,readword((unsigned char*)list+12)))return 0;
 }
 return 1;
}
int grid_card_observer_configure(ElfImage *im){
 stop=NULL;tip_rendered=NULL;glow_list_type=NULL;memset(cached_types,0,sizeof cached_types);
 if(!im||!im->relocated)return -1;
 uint32_t v,size;
 for(unsigned i=0;i<TYPE_COUNT;i++){
  if(elf_image_symbol(im,card_types[i].name,&v,&size)||v!=card_types[i].address||size!=4||!elf_image_contains(im,v,4,6))return -1;
  cached_types[i]=(void*)(im->base+v);
 }
 uint32_t a,b;
 if(elf_image_symbol(im,"__$gc_stop_threads",&a,&size)||size!=4||!elf_image_contains(im,a,4,6))return -1;
 if(elf_image_symbol(im,"_ZN3com8megacrit9cardcrawl7helpers9TipHelper20renderedTipThisFrameE",&b,&size)||size!=1||!elf_image_contains(im,b,1,6))return -1;
 uint32_t glow;
 if(elf_image_symbol(im,"_ZZN6System8Internal14GetRuntimeTypeIPNS_11Collections7Generic6List$1IPN3com8megacrit9cardcrawl3vfx9cardManip14CardGlowBorderEEEEEPNS0_11RuntimeTypeEvE6cached",&glow,&size)||size!=4||!elf_image_contains(im,glow,4,6))return -1;
 glow_list_type=(void*)(im->base+glow);
 stop=(void*)(im->base+a);tip_rendered=im->base+b;return 0;
}
