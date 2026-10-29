#include "grid_batch_contract.h"
#include <string.h>
static const unsigned offsets[]={20,24,28,72,80,84};
static uint32_t word(const unsigned char *p){uint32_t v;memcpy(&v,p,4);return v;}
static int supported(const unsigned char *b){
 /* Empty, actively drawing stock SpriteBatch; no custom shader. Class identity
  * and allocation size are checked separately against engine metadata. */
 return word(b)&&word(b+8)&&word(b+12)&&!word(b+16)&&b[32]==1&&!word(b+64);
}
static int stable(const unsigned char *a,const unsigned char *b){
 for(unsigned at=0;at<GRID_BATCH_BYTES;at++){
  int allowed=0;for(unsigned i=0;i<6;i++)if(at>=offsets[i]&&at<offsets[i]+4){allowed=1;break;}
  if(!allowed&&a[at]!=b[at])return 0;
 }
 return 1;
}
int grid_batch_prepare(GridBatchPatch *out,const unsigned char *first_before,const unsigned char *first_after,const unsigned char *incoming){
 if(!out||!first_before||!first_after||!incoming||!supported(first_before)||!supported(first_after)||!supported(incoming)||!stable(first_before,first_after)||!stable(first_after,incoming)||!word(first_after+20))return 0;
 uint32_t calls=word(first_after+84)-word(first_before+84);
 if(!calls||calls>4096)return 0;
 uint32_t before_count=word(first_before+80),after_count=word(first_after+80);
 /* begin() resets renderCalls, flush() increments both counters. With no
  * possible wrap, fewer final calls than total submissions proves a reset.
  * Ambiguous zero-start and wrap cases fall back instead of guessing. */
 if(before_count>UINT32_MAX-4096)return 0;
 int reset=after_count<calls;
 if(!reset&&(!before_count||after_count!=before_count+calls))return 0;
 GridBatchPatch p;
 for(unsigned i=0;i<4;i++)p.words[i]=word(first_after+offsets[i]);
 p.words[4]=reset?after_count:word(incoming+80)+calls;p.words[5]=word(incoming+84)+calls;
 *out=p;return 1;
}
void grid_batch_apply(unsigned char *batch,const GridBatchPatch *patch){for(unsigned i=0;i<6;i++)memcpy(batch+offsets[i],patch->words+i,4);}
