#include "elf_image.h"
#include <string.h>
static uint32_t word(const unsigned char *p){uint32_t v;memcpy(&v,p,4);return v;}
int elf_image_initializers(const ElfImage *im,uint32_t load,uint32_t *out,unsigned capacity,unsigned *count){
 if(!im||!im->relocated||!out||!count)return -1;
 uint32_t dynamic=0,bytes=0,init=0,array=0,size=0;unsigned seen=0;
 for(unsigned i=0;i<im->phnum;i++)if(im->phdrs[i].type==2){
  if(bytes)return -1;
  dynamic=im->phdrs[i].vaddr;bytes=im->phdrs[i].memsz;
 }
 if(!bytes||bytes%8||!elf_image_contains(im,dynamic,bytes,4))return -1;
 int terminated=0;
 for(uint32_t i=0;i<bytes;i+=8){
  uint32_t tag=word(im->base+dynamic+i),value=word(im->base+dynamic+i+4);
  if(!tag){terminated=1;break;}
  unsigned bit=tag==12?1:tag==25?2:tag==27?4:0;
  if(bit&&(seen&bit))return -1;
  seen|=bit;if(tag==12)init=value;else if(tag==25)array=value;else if(tag==27)size=value;
 }
 if(!terminated||size%4||size/4>128||((seen&6)!=0&&(seen&6)!=6))return -1;
 if(size&&!elf_image_contains(im,array,size,4))return -1;
 uint32_t pending[129];unsigned used=0;
 if(init)pending[used++]=init;
 for(uint32_t i=0;i<size;i+=4){
  uint32_t absolute=word(im->base+array+i);
  if(!absolute||absolute==UINT32_MAX)continue;
  if(absolute<load)return -1;
  pending[used++]=absolute-load;
 }
 if(used>capacity)return -1;
 for(unsigned i=0;i<used;i++)if(!elf_image_contains(im,pending[i]&~1u,2,5))return -1;
 memcpy(out,pending,used*sizeof(*out));*count=used;return 0;
}
