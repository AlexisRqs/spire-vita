#include "native_controller.h"
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
_Static_assert(sizeof(NativeControllerState)==28,"engine controller state ABI");
static NativeControllerState state;
static atomic_int opened;
static uint32_t stick_direction;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
void *native_controller_open(int index){if(index)return NULL;opened=1;return (void*)(uintptr_t)1;}
int native_controller_active(void){return opened;}
void native_controller_get(void *handle,NativeControllerState *out){
 if(!out)return;
 pthread_mutex_lock(&lock);
 if(handle==(void*)(uintptr_t)1)*out=state;else memset(out,0,sizeof(*out));
 pthread_mutex_unlock(&lock);
}
static float axis(unsigned v){int n=(int)(v>255?255:v)-128;return n>-24&&n<24?0:n/(n<0?128.0f:127.0f);}
int native_controller_input_held(uint32_t buttons,unsigned lx,unsigned ly,unsigned rx,unsigned ry){
 return buttons||axis(lx)!=0||axis(ly)!=0||axis(rx)!=0||axis(ry)!=0;
}
void native_controller_sample(uint32_t buttons,unsigned lx,unsigned ly,unsigned rx,unsigned ry,int blocked){
 /* Constructor table maps these bits to engine A/B/X/Y, Back, Start, LB/RB,
  * and virtual D-pad codes -2000,+2000,-2001,+2001. Six floats precede bits. */
 static const uint32_t masks[15]={0x4000,0x2000,0x8000,0x1000,1,0,8,0,0,0x100,0x200,0x10,0x40,0x80,0x20};
 /* The game treats analog axis callbacks differently from its D-pad.
  * Use one stable cardinal direction for the left navigation stick. */
 pthread_mutex_lock(&lock);
 int x=(int)(lx>255?255:lx)-128,y=(int)(ly>255?255:ly)-128;
 int ax=x<0?-x:x,ay=y<0?-y:y;
 uint32_t candidate=ax>ay?(x<0?0x80:0x20):(y<0?0x10:0x40);
 int strength=ax>ay?ax:ay;
 if(blocked||strength<40)stick_direction=0;
 else if(strength>=64){
  int old_strength=(stick_direction==0x80?-x:stick_direction==0x20?x:stick_direction==0x10?-y:stick_direction==0x40?y:0);
  if(!stick_direction||candidate==stick_direction||strength>old_strength+20)stick_direction=candidate;
 }
 /* Physical D-pad takes priority, including a deliberate diagonal. */
 if(!(buttons&0xf0))buttons|=stick_direction;
 NativeControllerState next={{0,0,axis(ry),axis(rx),!!(buttons&VITA_VIRTUAL_L2),!!(buttons&VITA_VIRTUAL_R2)},0};
 for(unsigned i=0;i<15;i++)if(buttons&masks[i])next.buttons|=1u<<i;
 if(blocked)memset(&next,0,sizeof(next));
 state=next;pthread_mutex_unlock(&lock);
}
int native_controller_configure(ElfImage *im){
 const char *names[]={"nativePs4_openDevice","nativePs4_getState"};
 uintptr_t targets[]={(uintptr_t)native_controller_open,(uintptr_t)native_controller_get};uint32_t slots[2];
 if(!im||!im->relocated)return -1;
 for(unsigned i=0;i<2;i++){uint32_t v,n,current;
  if(elf_image_symbol(im,names[i],&v,&n)||elf_image_relocation_target(im,names[i],22,&slots[i])||(slots[i]&3)||!elf_image_contains(im,slots[i],4,2)||!elf_image_contains(im,v&~1u,2,5))return -1;
  memcpy(&current,im->base+slots[i],4);if(current!=(uint32_t)(uintptr_t)(im->base+v))return -1;
 }
 for(unsigned i=0;i<2;i++){uint32_t t=targets[i];memcpy(im->base+slots[i],&t,4);}return 0;
}
