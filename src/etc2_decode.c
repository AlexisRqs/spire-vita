#include "etc2_decode.h"
#include <stdbool.h>
#include <string.h>
extern bool detexDecompressBlockETC2(const uint8_t *,uint32_t,uint32_t,uint8_t *);
extern bool detexDecompressBlockETC2_PUNCHTHROUGH(const uint8_t *,uint32_t,uint32_t,uint8_t *);
int etc2_decode_rgba(unsigned format,unsigned width,unsigned height,const void *input,size_t input_size,void *output,size_t output_size){
 if((format!=0x9274&&format!=0x9276)||!width||!height||width>4096||height>4096||!input||!output)return -1;
 size_t bx=(width+3)/4,by=(height+3)/4;
 if(input_size!=bx*by*8||output_size<(size_t)width*height*4)return -1;
 const uint8_t *in=input;uint8_t *out=output;
 for(size_t y=0;y<by;y++)for(size_t x=0;x<bx;x++){
  uint32_t block[16];
  bool ok=format==0x9274?detexDecompressBlockETC2(in,~0u,0,(uint8_t *)block):detexDecompressBlockETC2_PUNCHTHROUGH(in,~0u,0,(uint8_t *)block);
  if(!ok)return -1;
  in+=8;
  unsigned columns=width-x*4<4?width-x*4:4;
  for(unsigned row=0;row<4&&y*4+row<height;row++)memcpy(out+((y*4+row)*width+x*4)*4,block+row*4,columns*4);
 }
 return 0;
}
