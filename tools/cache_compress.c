#include <stdint.h>
#include <string.h>
extern void stb_compress_dxt_block(unsigned char*,const unsigned char*,int,int);
void cache_compress(const uint32_t *src,unsigned w,unsigned h,int alpha,uint8_t *out){
 for(unsigned y=0;y<h;y+=4)for(unsigned x=0;x<w;x+=4){uint32_t tile[16];
 for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)tile[r*4+c]=src[(y+r<h?y+r:h-1)*w+(x+c<w?x+c:w-1)];
 stb_compress_dxt_block(out,(void*)tile,alpha,2);out+=alpha?16:8;}
}
