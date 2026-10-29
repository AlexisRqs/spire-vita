#include "keyboard.h"
#include <string.h>
static KeyboardBackend backend;
static uint16_t title_buffer[129],initial_buffer[KEYBOARD_CAPACITY],result_buffer[KEYBOARD_CAPACITY],input_buffer[2*KEYBOARD_CAPACITY+1];
static int active,failed,release_gate;
static void copy16(uint16_t *dst,const uint16_t *src,unsigned capacity){
 unsigned n=0;while(src&&n+1<capacity&&src[n]){dst[n]=src[n];n++;}
 /* Never truncate between UTF-16 surrogate halves. */
 if(n&&dst[n-1]>=0xd800&&dst[n-1]<=0xdbff)n--;
 dst[n]=0;
}
void keyboard_bind(KeyboardBackend b){backend=b;}
int keyboard_active(void){return active;}
int keyboard_gate_input(int held){
 if(active||failed){release_gate=1;return 1;}
 if(release_gate){if(!held)release_gate=0;return 1;}
 return 0;
}
void keyboard_show(const uint16_t *title,const uint16_t *initial){
 if(active||failed)return;
 copy16(title_buffer,title,129);copy16(initial_buffer,initial,KEYBOARD_CAPACITY);
 memset(result_buffer,0,sizeof(result_buffer));memset(input_buffer,0,sizeof(input_buffer));release_gate=1;
 if(!backend.begin||!backend.poll||!backend.end||backend.begin(title_buffer,initial_buffer,input_buffer,KEYBOARD_CAPACITY)<0){failed=1;return;}
 active=1;
}
int keyboard_update(const uint16_t **output){
 if(output)*output=NULL;
 if(failed){failed=0;return 1;}
 if(!active)return 0;
 int status=backend.poll();if(!status)return 0;
 backend.end();active=0;release_gate=1;
 copy16(result_buffer,input_buffer,KEYBOARD_CAPACITY);
 if(status==1&&output)*output=result_buffer;
 return 1;
}
int keyboard_configure(ElfImage *im){
 const char *names[]={"nativePs4_ShowKeyboard","nativePs4_UpdateKeyboard"};
 uintptr_t functions[]={(uintptr_t)keyboard_show,(uintptr_t)keyboard_update};uint32_t slots[2];
 if(!im||!im->relocated)return -1;
 for(unsigned i=0;i<2;i++){
  uint32_t value,size,current;
  if(elf_image_symbol(im,names[i],&value,&size)||elf_image_relocation_target(im,names[i],22,&slots[i])||
   (slots[i]&3)||!elf_image_contains(im,slots[i],4,2)||!elf_image_contains(im,value&~1u,2,5))return -1;
  memcpy(&current,im->base+slots[i],4);if(current!=(uint32_t)(uintptr_t)(im->base+value))return -1;
 }
 for(unsigned i=0;i<2;i++){uint32_t target=functions[i];memcpy(im->base+slots[i],&target,4);}return 0;
}
