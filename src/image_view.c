#include "image_view.h"
#include <string.h>
#include <stdatomic.h>
static atomic_uint view_count,avoided_bytes;
void image_view_stats(unsigned *views,unsigned *bytes){*views=atomic_load(&view_count);*bytes=atomic_load(&avoided_bytes);}
static ImageLoad load;
static ImageAlloc allocate;
static ImageBase owner_of;
static ImageSize size_of;
void image_view_bind(ImageLoad original,ImageAlloc alloc,ImageBase base,ImageSize size){load=original;allocate=alloc;owner_of=base;size_of=size;}
/* ARM32 BRUTE byte-array view for this verified engine layout.
 * A scanned GC allocation holds an explicit owner reference at word7.
 * This avoids relying on interior-pointer discovery for the source lifetime. */
void *image_view_load(const unsigned char *data,int length,int *w,int *h,int *format,void **out){
 if(!load)return NULL;
 if(!data||length<68||!allocate||!owner_of||!size_of||!w||!h||!format||!out)goto fallback;
 static const unsigned char magic[12]={0xab,'K','T','X',' ', '1','1',0xbb,13,10,26,10};
 uint32_t header[13];memcpy(header,data+12,sizeof(header));
 if(memcmp(data,magic,12)||header[0]!=0x04030201||header[12]>(uint32_t)length-68)goto fallback;
 uint32_t start=68+header[12],bytes;memcpy(&bytes,data+start-4,4);
 if(bytes>(uint32_t)length-start||bytes!=(uint32_t)length-start)goto fallback;
 void *owner=owner_of((void *)data);
 if(!owner||(uintptr_t)owner>(uintptr_t)data)goto fallback;
 size_t offset=(uintptr_t)data-(uintptr_t)owner,capacity=size_of(owner);
 if(offset>capacity||(size_t)length>capacity-offset)goto fallback;
 unsigned char empty[68];memcpy(empty,data,64);memset(empty+60,0,8);
 void *prototype=NULL;load(empty,68,w,h,format,&prototype);
 if(!prototype)goto fallback;
 /* Verify the original runtime layout before constructing a view. */
 uint32_t *p=prototype;
 if(p[3]!=1||p[4]!=1||p[5]!=(uint32_t)(uintptr_t)(p+6)||p[6]!=0)goto fallback;
 uint32_t *view=allocate(32);if(!view)goto fallback;
 memcpy(view,p,24);view[2]=(uint32_t)(uintptr_t)(data+start);
 view[5]=(uint32_t)(uintptr_t)(view+6);view[6]=bytes;view[7]=(uint32_t)(uintptr_t)owner;
 atomic_fetch_add(&view_count,1);atomic_fetch_add(&avoided_bytes,bytes);
 *out=view;return view;
fallback:
 return load(data,length,w,h,format,out);
}

int image_view_configure(ElfImage *im){
 const char *names[]={"nativePs4_loadImage","GC_malloc","GC_base","GC_size"};
 uint32_t value[4],size,slot,current;
 if(!im||!im->relocated)return -1;
 for(unsigned i=0;i<4;i++)if(elf_image_symbol(im,names[i],&value[i],&size)||!elf_image_contains(im,value[i]&~1u,2,5))return -1;
 if(elf_image_relocation_target(im,names[0],22,&slot)||(slot&3)||!elf_image_contains(im,slot,4,2))return -1;
 memcpy(&current,im->base+slot,4);
 if(current!=(uint32_t)(uintptr_t)(im->base+value[0]))return -1;
 image_view_bind((ImageLoad)(im->base+value[0]),(ImageAlloc)(im->base+value[1]),(ImageBase)(im->base+value[2]),(ImageSize)(im->base+value[3]));
 uint32_t target=(uint32_t)(uintptr_t)image_view_load;memcpy(im->base+slot,&target,4);return 0;
}
