#include "grid_objects.h"
#include "android_log.h"
#include <string.h>
#define OBJECT_LIMIT 1024u
static void **types[2];
static uint32_t *gc_stop;
static unsigned char *peeking;
typedef struct {void *object;unsigned size,valid;unsigned char bytes[OBJECT_LIMIT];} Snapshot;
static Snapshot before[2],first_before[2],first_after[2];
static int quiet;
static unsigned active,ordinal,current_frame,first_frame,first_valid;
static const char *labels[]={"grid","batch"};
static uint32_t getword(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static int capture(Snapshot *s,void *object,unsigned kind){
 s->valid=0;
 if(!object||!gc_stop||*gc_stop||!types[kind]||!*types[kind])return 0;
 void *type=*types[kind];
 if(getword(object)!=(uint32_t)((uintptr_t)type+0x38))return 0;
 /* Exact ClassHelpers::Alloc reads the allocation size at RuntimeType+0.
  * The expected type comes from the engine's own validated cached symbol. */
 unsigned size=getword(type);
 if(size<4||size>OBJECT_LIMIT)return 0;
 s->object=object;s->size=size;memcpy(s->bytes,object,size);s->valid=1;return 1;
}
static int comparable(const Snapshot *a,const Snapshot *b){return a->valid&&b->valid&&a->object==b->object&&a->size==b->size;}
static void difference(const Snapshot *a,const Snapshot *b,unsigned kind,const char *phase){
 if(quiet)return;
 if(!comparable(a,b)){android_log_print(4,"GridObjects","frame=%u object=%s phase=%s comparable=0",current_frame,labels[kind],phase);return;}
 unsigned changed=0,reported=0;
 for(unsigned at=0;at<a->size;at+=4){
  unsigned n=a->size-at<4?a->size-at:4;
  if(memcmp(a->bytes+at,b->bytes+at,n)){
   changed++;
   if(reported++<16)android_log_print(4,"GridObjectField","frame=%u object=%s phase=%s offset=%u bytes=%u",current_frame,labels[kind],phase,at,n);
  }
 }
 android_log_print(4,"GridObjects","frame=%u object=%s phase=%s comparable=1 size=%u changed_words=%u shallow_only=1",current_frame,labels[kind],phase,a->size,changed);
}
void grid_objects_begin(void *grid,void *batch,unsigned pass,unsigned frame,int sample){
 active=0;
 if(!sample||pass<1||pass>2)return;
 ordinal=pass;current_frame=frame;active=1;quiet=sample>1;
 if(pass==1){first_valid=0;first_frame=frame;}
 capture(before,grid,0);capture(before+1,batch,1);
 if(pass==1){first_before[0]=before[0];first_before[1]=before[1];}
 if(pass==2){
  if(!first_valid||first_frame!=frame){first_after[0].valid=first_after[1].valid=0;}
  for(unsigned i=0;i<2;i++)difference(first_after+i,before+i,i,"between");
 }
}
void grid_objects_end(void){
 if(!active)return;
 active=0;
 for(unsigned i=0;i<2;i++){
  Snapshot after={0};
  if(before[i].valid)capture(&after,before[i].object,i);
  difference(before+i,&after,i,ordinal==1?"first":"second");
  if(ordinal==1)first_after[i]=after;
 }
 if(ordinal==1)first_valid=first_after[0].valid&&first_after[1].valid;
}
int grid_objects_plain(void *grid,void *batch){
 Snapshot g={0},b={0};
 if(!peeking||*peeking||!capture(&g,grid,0)||!capture(&b,batch,1)||g.size!=112||b.size!=GRID_BATCH_BYTES)return 0;
 return !g.bytes[0x35]&&!g.bytes[0x36]&&!g.bytes[0x51]&&!g.bytes[0x52];
}
int grid_objects_prepare_replay(void *grid,void *batch,unsigned frame,GridBatchPatch *patch){
 if(!active||ordinal!=2||!first_valid||first_frame!=frame||current_frame!=frame||!peeking||*peeking||!gc_stop||*gc_stop)return 0;
 if(before[0].object!=grid||before[1].object!=batch||before[0].size!=112||before[1].size!=GRID_BATCH_BYTES)return 0;
 for(unsigned i=0;i<2;i++)if(!comparable(first_before+i,first_after+i)||!comparable(first_after+i,before+i)||memcmp(before[i].bytes,before[i].object,before[i].size))return 0;
 /* The preview branch at0x2198a38 is selected by byte0x35. Exclude it
  * explicitly even when its animation happens to produce identical bytes. */
 const unsigned char *g=before[0].bytes;
 if(g[0x35]||g[0x36]||g[0x51]||g[0x52]||memcmp(first_before[0].bytes,first_after[0].bytes,112)||memcmp(first_after[0].bytes,g,112))return 0;
 return grid_batch_prepare(patch,first_before[1].bytes,first_after[1].bytes,before[1].bytes);
}
int grid_objects_configure(ElfImage *im){
 static const char *const names[]={
 "_ZZN6System8Internal14GetRuntimeTypeIPN3com8megacrit9cardcrawl7screens6select20GridCardSelectScreenEEEPNS0_11RuntimeTypeEvE6cached",
 "_ZZN6System8Internal14GetRuntimeTypeIPN3com8badlogic3gdx8graphics3g2d11SpriteBatchEEEPNS0_11RuntimeTypeEvE6cached",
 "__$gc_stop_threads"};
 uint32_t addresses[3],size,v;
 types[0]=types[1]=NULL;gc_stop=NULL;peeking=NULL;
 if(!im||!im->relocated)return -1;
 /* Exact allocator version, including its size load and header assignment. */
 static const unsigned char prefix[]={0x10,0xb5,0x81,0x88,0x04,0x46,0x00,0x68};
 static const unsigned char header[]={0x04,0xf1,0x38,0x01,0x01,0x60};
 if(elf_image_symbol(im,"_ZN6System8Internal12ClassHelpers5AllocEPNS0_11RuntimeTypeE",&v,&size)||v!=0x1b6f81d||size!=46||!elf_image_contains(im,v&~1u,size,5)||memcmp(im->base+(v&~1u),prefix,sizeof prefix)||memcmp(im->base+0x1b6f832,header,sizeof header))return -1;
 for(unsigned i=0;i<3;i++)if(elf_image_symbol(im,names[i],addresses+i,&size)||size!=4||!elf_image_contains(im,addresses[i],4,6))return -1;
 uint32_t peek;
 if(elf_image_symbol(im,"_ZN3com8megacrit9cardcrawl2ui7buttons10PeekButton9isPeekingE",&peek,&size)||size!=1||!elf_image_contains(im,peek,1,6))return -1;
 peeking=im->base+peek;
 types[0]=(void*)(im->base+addresses[0]);types[1]=(void*)(im->base+addresses[1]);gc_stop=(void*)(im->base+addresses[2]);return 0;
}
