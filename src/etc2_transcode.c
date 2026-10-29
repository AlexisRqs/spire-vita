#include "etc2_transcode.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
extern bool detexDecompressBlockETC2(const uint8_t *,uint32_t,uint32_t,uint8_t *);
extern bool detexDecompressBlockETC2_PUNCHTHROUGH(const uint8_t *,uint32_t,uint32_t,uint8_t *);
extern bool detexDecompressBlockETC2_EAC(const uint8_t *,uint32_t,uint32_t,uint8_t *);
/* Pinned vitaGL exports stb_dxt; 2 = STB_DXT_HIGHQUAL. */
extern void stb_compress_dxt_block(unsigned char *,const unsigned char *,int,int);
int etc2_transcode(unsigned format,unsigned width,unsigned height,const void *input,size_t input_size,void *output,size_t output_size){
 if((format!=0x9274&&format!=0x9276&&format!=0x9278)||!width||!height||width>4096||height>4096||!input||!output)return -1;
 size_t bx=(width+3)/4,by=(height+3)/4,src_bytes=format==0x9278?16:8,dst_bytes=format==0x9274?8:16;
 if(input_size!=bx*by*src_bytes||output_size<bx*by*dst_bytes)return -1;
 const uint8_t *in=input;uint8_t *out=output;
 for(size_t y=0;y<by;y++)for(size_t x=0;x<bx;x++){
  uint32_t tile[16];uint8_t *rgba=(uint8_t *)tile;
  bool ok=format==0x9274?detexDecompressBlockETC2(in,~0u,0,rgba):format==0x9276?detexDecompressBlockETC2_PUNCHTHROUGH(in,~0u,0,rgba):detexDecompressBlockETC2_EAC(in,~0u,0,rgba);
  if(!ok)return -1;
  unsigned cols=width-x*4<4?width-x*4:4,rows=height-y*4<4?height-y*4:4;
  for(unsigned r=0;r<rows;r++)for(unsigned c=cols;c<4;c++)tile[r*4+c]=tile[r*4+cols-1];
  for(unsigned r=rows;r<4;r++)memcpy(tile+r*4,tile+(rows-1)*4,16);
  stb_compress_dxt_block(out,rgba,format!=0x9274,2);
  in+=src_bytes;out+=dst_bytes;
 }
 return 0;
}
