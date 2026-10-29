#include "preview_audio.h"
#include <string.h>
/* Original APK getenv always returns NULL. Select only its built-in OpenAL
 * null backend for this silent preview, leaving all other queries unchanged. */
char *preview_audio_getenv(const char *name){
 static char driver[]="null";
 return name&&!strcmp(name,"ALSOFT_DRIVERS")?driver:NULL;
}
int preview_audio_configure(ElfImage *im){
 uint32_t slot,value,size,current;
 if(!im||!im->relocated||elf_image_symbol(im,"getenv",&value,&size)||
    elf_image_relocation_target(im,"getenv",22,&slot)||
    (slot&3)||!elf_image_contains(im,slot,4,2)||
    !elf_image_contains(im,value&~1u,2,5))return -1;
 memcpy(&current,im->base+slot,4);
 if(current!=(uint32_t)(uintptr_t)(im->base+value))return -1;
 uint32_t target=(uint32_t)(uintptr_t)preview_audio_getenv;
 memcpy(im->base+slot,&target,4);return 0;
}
