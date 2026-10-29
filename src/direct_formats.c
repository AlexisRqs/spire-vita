#include "direct_formats.h"
#include <string.h>
static int (*original_format)(int);
static void *(*original_enum)(int);
int direct_to_gl_format(int f){return f==0x83f0||f==0x83f3?f:original_format(f);}
void *direct_from_format(int f){return original_enum(f==0x83f0?0x9274:f==0x83f3?0x9278:f);}
int direct_formats_configure(ElfImage *im){
 const char *names[]={"_ZN3com8badlogic3gdx8graphics3g2d26Gdx2DPixmap$_$S_toGlFormatEi","_ZN3com8badlogic3gdx8graphics39Pixmap$Format$_$S_fromGdx2DPixmapFormatEi"};
 const unsigned char signatures[2][8]={{0xb0,0xb5,0x82,0xb0,0x28,0x49,0x79,0x44},{0xb0,0xb5,0x82,0xb0,0x04,0x46,0x2f,0x48}};
 const unsigned expected[]={0x1da31a1,0x1dcab39};
 uint32_t values[2],slots[2],size,current;
 if(!im||!im->relocated)return -1;
 for(unsigned i=0;i<2;i++){
  if(elf_image_symbol(im,names[i],values+i,&size)||values[i]!=expected[i]||!elf_image_contains(im,values[i]&~1u,8,5)||
   elf_image_relocation_target(im,names[i],22,slots+i)||(slots[i]&3)||!elf_image_contains(im,slots[i],4,2))return -1;
  if(memcmp(im->base+(values[i]&~1u),signatures[i],8))return -1;
  memcpy(&current,im->base+slots[i],4);if(current!=(uint32_t)(uintptr_t)(im->base+values[i]))return -1;
 }
 original_format=(void*)(im->base+values[0]);original_enum=(void*)(im->base+values[1]);
 const uintptr_t hooks[]={(uintptr_t)direct_to_gl_format,(uintptr_t)direct_from_format};
 for(unsigned i=0;i<2;i++){uint32_t p=hooks[i];memcpy(im->base+slots[i],&p,4);}return 0;
}
