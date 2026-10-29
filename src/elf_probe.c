#include "elf_probe.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static uint16_t u16(const unsigned char *p) { return p[0] | ((uint16_t)p[1]<<8); }
static uint32_t u32(const unsigned char *p) { return u16(p) | ((uint32_t)u16(p+2)<<16); }
static int read_at(FILE *f, uint64_t offset, void *b, unsigned n, uint64_t size) {
 return offset <= size && n <= size-offset && offset <= 0x7fffffff &&
        fseek(f,(long)offset,SEEK_SET)==0 && fread(b,1,n,f)==n;
}
int elf_probe(const char *path, ElfProbe *out, char *error, unsigned cap) {
 FILE *f=fopen(path,"rb"); unsigned char h[52], p[40];
 const char *why="invalid or truncated ELF"; int result=-1;
 memset(out,0,sizeof(*out));
 if(!f) {snprintf(error,cap,"cannot open engine");return -1;}
 if(fseek(f,0,SEEK_END))goto end;
 long length=ftell(f);if(length<52)goto end;uint64_t size=(uint64_t)length;
 if(!read_at(f,0,h,52,size))goto end;
 if(memcmp(h,"\177ELF",4)||h[4]!=1||h[5]!=1||h[6]!=1||u16(h+16)!=3||u16(h+18)!=40){why="requires little-endian ARM32 ET_DYN";goto end;}
 uint32_t ph=u32(h+28),sh=u32(h+32),low=UINT32_MAX,high=0;
 unsigned pn=u16(h+44),sn=u16(h+48);
 if(u16(h+42)!=32||u16(h+46)!=40||!pn||pn>64||!sn){why="unsupported ELF table layout";goto end;}
 for(unsigned i=0;i<pn;i++) {
  if(!read_at(f,(uint64_t)ph+i*32,p,32,size))goto end;
  uint32_t t=u32(p),off=u32(p+4),va=u32(p+8),fs=u32(p+16),ms=u32(p+20);
  if(t!=1&&t!=7)continue;
  if(fs>ms||(uint64_t)off+fs>size||(uint64_t)va+ms>UINT32_MAX)goto end;
  if(t==1){out->load_segments++;if(va<low)low=va;if(va+ms>high)high=va+ms;}
  else {out->tls_memsz=ms;out->tls_align=u32(p+28);}
 }
 if(!out->load_segments)goto end;
 out->image_span=high-low;
 for(unsigned i=0;i<sn;i++) {
  if(!read_at(f,(uint64_t)sh+i*40,p,40,size))goto end;
  if(u32(p+4)!=9)continue; /* SHT_REL */
  uint32_t off=u32(p+16),bytes=u32(p+20),stride=u32(p+36);
  if(stride!=8||bytes%8||(uint64_t)off+bytes>size)goto end;
  unsigned char rel[8];
  for(uint32_t j=0;j<bytes;j+=8) {
   if(!read_at(f,(uint64_t)off+j,rel,8,size))goto end;
   unsigned type=u32(rel+4)&255;
   if(type==17)out->tls_module_relocs++;
   else if(type==18)out->tls_offset_relocs++;
   else out->other_relocs++;
  }
 }
 result=0;
end:
 fclose(f);if(result)snprintf(error,cap,"%s",why);else if(cap)error[0]=0;return result;
}
