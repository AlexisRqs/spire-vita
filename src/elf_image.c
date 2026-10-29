#include "elf_image.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static uint16_t u16(const unsigned char *p) { return p[0] | ((uint16_t)p[1]<<8); }
static uint32_t u32(const unsigned char *p) { return u16(p) | ((uint32_t)u16(p+2)<<16); }
static int read_at(FILE *f, uint64_t off, void *out, uint32_t size, uint64_t length) {
 return off <= length && size <= length-off && off <= 0x7fffffff &&
        !fseek(f, (long)off, SEEK_SET) && fread(out, 1, size, f) == size;
}
int elf_image_contains(const ElfImage *im, uint32_t addr, uint32_t size, uint32_t flags) {
 for (uint32_t i=0; i<im->count; i++) {
  const ImageSegment *s=&im->segments[i];
  if ((s->flags & flags)==flags && addr>=s->start &&
      (uint64_t)addr+size <= (uint64_t)s->start+s->size) return 1;
 }
 return 0;
}
int elf_image_load(const char *path, void *buffer, uint32_t capacity,
                   ElfImage *im, char *error, unsigned cap) {
 memset(im,0,sizeof(*im));
 FILE *f=fopen(path,"rb");
 const char *why="invalid image layout"; int result=-1;
 unsigned char h[52],p[40];
 if (!f) { snprintf(error,cap,"cannot open engine"); return -1; }
 if (!buffer || !capacity || capacity>64*1024*1024 || fseek(f,0,SEEK_END)) goto end;
 long len=ftell(f); if (len<52) goto end;
 uint64_t length=(uint64_t)len;
 if (!read_at(f,0,h,52,length) || memcmp(h,"\177ELF\1\1\1",7) ||
     u16(h+16)!=3 || u16(h+18)!=40 || u16(h+42)!=32 || u16(h+46)!=40) goto end;
 uint32_t ph=u32(h+28), sh=u32(h+32); unsigned pn=u16(h+44),sn=u16(h+48);
 if (!pn || pn>64 || !sn) goto end;
 im->base=buffer; im->capacity=capacity;
 memset(buffer,0,capacity);
 for (unsigned i=0;i<pn;i++) {
  if (!read_at(f,(uint64_t)ph+i*32,p,32,length)) goto end;
  im->phdrs[i]=(AndroidPhdr){u32(p),u32(p+4),u32(p+8),u32(p+12),u32(p+16),u32(p+20),u32(p+24),u32(p+28)};
  im->phnum=i+1;
  if (u32(p)==7) {
   if (im->tls_size) goto end;
   im->tls_size=u32(p+20); im->tls_align=u32(p+28); im->tls_filesz=u32(p+16);
   if (!im->tls_size || im->tls_size>4096 || im->tls_filesz>im->tls_size ||
       !im->tls_align || im->tls_align>4096 || (im->tls_align&(im->tls_align-1))) goto end;
   /* Current engine has zero initialized TLS. Reject other layouts for now. */
   if (im->tls_filesz) goto end;
  }
  if (u32(p)!=1) continue;
  uint32_t off=u32(p+4),addr=u32(p+8),fs=u32(p+16),ms=u32(p+20),align=u32(p+28);
  if (im->count==16 || !ms || fs>ms || addr>capacity || ms>capacity-addr ||
      (align>1 && ((align&(align-1)) || (addr%align)!=(off%align)))) goto end;
  for (uint32_t j=0;j<im->count;j++) {
   ImageSegment *s=&im->segments[j];
   if ((uint64_t)addr+ms>s->start && (uint64_t)s->start+s->size>addr) goto end;
  }
  if (!read_at(f,off,im->base+addr,fs,length)) goto end;
  im->segments[im->count++]=(ImageSegment){addr,ms,u32(p+24)};
 }
 if (!im->count) goto end;
 unsigned symbol_section=0;
 for (unsigned i=0;i<sn;i++) {
  if (!read_at(f,(uint64_t)sh+i*40,p,40,length)) goto end;
  if (u32(p+4)!=11) continue; /* SHT_DYNSYM */
  uint32_t addr=u32(p+12),size=u32(p+20),link=u32(p+24);
  if (im->symbols || u32(p+36)!=16 || size%16 || link>=sn ||
      !elf_image_contains(im,addr,size,4)) goto end;
  im->symbols=im->base+addr; im->symbols_size=size;
  symbol_section=i;
  if (!read_at(f,(uint64_t)sh+link*40,p,40,length) || u32(p+4)!=3) goto end;
  addr=u32(p+12); size=u32(p+20);
  if (!size || !elf_image_contains(im,addr,size,4)) goto end;
  im->strings=im->base+addr; im->strings_size=size;
 }
 if (!im->symbols || !im->strings) goto end;
 for (unsigned i=0;i<sn;i++) {
  if (!read_at(f,(uint64_t)sh+i*40,p,40,length)) goto end;
  if (u32(p+4)!=9) continue;
  uint32_t addr=u32(p+12),size=u32(p+20);
  if (im->reloc_count==8 || u32(p+36)!=8 || size%8 ||
      u32(p+24)!=symbol_section || !elf_image_contains(im,addr,size,4)) goto end;
  im->relocs[im->reloc_count++]=(ImageRelocs){im->base+addr,size};
 }
 result=0;
end:
 fclose(f);
 if (result) { memset(im,0,sizeof(*im)); snprintf(error,cap,"%s",why); }
 else if (cap) error[0]=0;
 return result;
}
/* Startup-only index. The mapped symbol/string tables must remain immutable.
 * No engine or GC objects are retained; release before entering the game. */
static const ElfImage *indexed_image;
static uint32_t *symbol_index, index_mask;
static uint32_t name_hash(const char *s) {
 uint32_t h=2166136261u;
 while (*s) { h^=(unsigned char)*s++; h*=16777619u; }
 return h;
}
void elf_image_index_release(void) {
 free(symbol_index); symbol_index=NULL; indexed_image=NULL; index_mask=0;
}
int elf_image_index_prepare(const ElfImage *im) {
 elf_image_index_release();
 if (!im || !im->symbols || !im->strings || im->symbols_size%16 ||
     im->symbols_size/16>1000000) return -1;
 uint32_t count=im->symbols_size/16, capacity=2;
 while (capacity<count*2) capacity*=2;
 uint32_t *table=calloc(capacity,sizeof(*table));
 if (!table) return 1; /* Original linear lookup remains available. */
 for (uint32_t i=0;i<count;i++) {
  const unsigned char *s=im->symbols+i*16;
  uint32_t off=u32(s);
  if (off>=im->strings_size || !memchr(im->strings+off,0,im->strings_size-off)) {
   free(table); return -1;
  }
  if (!u16(s+14)) continue;
  const char *name=(const char *)im->strings+off;
  uint32_t slot=name_hash(name)&(capacity-1);
  while (table[slot]) {
   uint32_t other=u32(im->symbols+(table[slot]-1)*16);
   if (!strcmp((const char *)im->strings+other,name)) break;
   slot=(slot+1)&(capacity-1);
  }
  if (!table[slot]) table[slot]=i+1; /* Preserve first defined match. */
 }
 symbol_index=table; index_mask=capacity-1; indexed_image=im;
 return 0;
}
int elf_image_symbol(const ElfImage *im,const char *name,uint32_t *value,uint32_t *size) {
 if (!im->symbols || !im->strings) return -1;
 if (symbol_index && indexed_image==im) {
  uint32_t slot=name_hash(name)&index_mask;
  while (symbol_index[slot]) {
   const unsigned char *s=im->symbols+(symbol_index[slot]-1)*16;
   if (!strcmp((const char *)im->strings+u32(s),name)) {
    *value=u32(s+4); *size=u32(s+8); return 0;
   }
   slot=(slot+1)&index_mask;
  }
  return -1;
 }
 for (uint32_t i=0;i<im->symbols_size;i+=16) {
  const unsigned char *s=im->symbols+i;
  uint32_t off=u32(s);
  if (off>=im->strings_size) return -1;
  const char *text=(const char *)im->strings+off;
  if (!memchr(text,0,im->strings_size-off)) return -1;
  if (u16(s+14) && !strcmp(text,name)) { *value=u32(s+4); *size=u32(s+8); return 0; }
 }
 return -1;
}
