#include "elf_image.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static uint32_t word(const unsigned char *p) {
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void put(unsigned char *p,uint32_t v) {
 for (unsigned i=0;i<4;i++) p[i]=(unsigned char)(v>>(i*8));
}
typedef struct { uint32_t address,value; } Patch;
int elf_image_relocation_target(const ElfImage *im,const char *name,uint32_t type,uint32_t *target) {
 for (uint32_t i=0;i<im->reloc_count;i++) for (uint32_t j=0;j<im->relocs[i].size;j+=8) {
  const unsigned char *r=im->relocs[i].data+j;
  uint32_t info=word(r+4),symbol=info>>8;
  if ((info&255)!=type || symbol>=im->symbols_size/16) continue;
  uint32_t off=word(im->symbols+symbol*16);
  if (off>=im->strings_size || !memchr(im->strings+off,0,im->strings_size-off)) return -1;
  if (!strcmp((const char *)im->strings+off,name)) { *target=word(r); return 0; }
 }
 return -1;
}
int elf_image_relocate(ElfImage *im,uint32_t base,ImageResolve resolve,void *context,
                       RelocReport *report,char *error,unsigned cap) {
 memset(report,0,sizeof(*report));
 const char *why="invalid relocation";
 Patch *patches=NULL; int rc=-1; uint32_t count=0,used=0;
 if (im->relocated || (uint64_t)base+im->capacity>0x100000000ULL) goto done;
 for (uint32_t i=0;i<im->reloc_count;i++) {
  if (im->relocs[i].size%8 || im->relocs[i].size/8>1000000-count) goto done;
  count+=im->relocs[i].size/8;
 }
 patches=calloc(count?count:1,sizeof(*patches));
 if (!patches) { why="relocation allocation failed"; goto done; }
 for (uint32_t i=0;i<im->reloc_count;i++) for (uint32_t j=0;j<im->relocs[i].size;j+=8) {
  const unsigned char *r=im->relocs[i].data+j;
  uint32_t target=word(r),info=word(r+4),type=info&255,sym=info>>8;
  if (!type) continue;
  if ((target&3) || !elf_image_contains(im,target,4,2) || sym>=im->symbols_size/16) goto done;
  const unsigned char *s=im->symbols+sym*16;
  uint32_t value=word(s+4),name=word(s),section=s[14]|((uint32_t)s[15]<<8),kind=s[12]&15;
  uint32_t addend=word(im->base+target),out=0;
  if (type==23) { /* R_ARM_RELATIVE, this loader supports symbol index zero. */
   if (sym) goto done;
   out=base+addend;
  } else if (type==17 || type==18) {
   if (!im->tls_size || (sym && (!section || kind!=6 || value>=im->tls_size))) goto done;
   if (type==17) out=1;
   else {
    if (!sym || (uint64_t)value+addend>=im->tls_size) goto done;
    out=value+addend;
   }
   report->tls++;
  } else if (type==2 || type==21 || type==22) {
   uint32_t resolved;
   if (section) {
    if (kind==6 || section==0xfff2) goto done;
    if (section==0xfff1) resolved=value; /* SHN_ABS */
    else {
     uint32_t address=kind==2 ? value&~1u : value;
     if (!elf_image_contains(im,address,0,0)) goto done;
     resolved=base+value;
    }
   } else {
    if (name>=im->strings_size || !memchr(im->strings+name,0,im->strings_size-name)) goto done;
    if (resolve && resolve((const char *)im->strings+name,&resolved,context)) report->bound++;
    else { report->unresolved++; patches[used++]=(Patch){target,0}; continue; }
   }
   /* AAELF32: REL JUMP_SLOT's A is zero, not the lazy-binding GOT word. */
   if (type==22) out=resolved;
   else if (kind==2) out=((resolved&~1u)+addend)|(resolved&1u);
   else out=resolved+addend;
  } else { why="unsupported ARM relocation"; goto done; }
  patches[used++]=(Patch){target,out}; report->applied++;
 }
 /* Do not partially modify the image when validation fails. */
 for (uint32_t i=0;i<used;i++) put(im->base+patches[i].address,patches[i].value);
 im->relocated=1; rc=0;
done:
 free(patches);
 if (rc) { memset(report,0,sizeof(*report)); snprintf(error,cap,"%s",why); }
 else if (cap) error[0]=0;
 return rc;
}
