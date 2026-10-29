#include "grid_scalar_state.h"
#include <string.h>
void grid_scalar_reset(GridScalarState *s){if(++s->epoch==0){memset(s,0,sizeof *s);s->epoch=1;}s->used=0;s->valid=1;}
int grid_scalar_enable(GridScalarState *s,uint32_t cap,int enabled){
 if(!s->valid)return 0;
 if(!cap){s->valid=0;return 0;}
 unsigned free_slot=GRID_SCALAR_SLOTS;
 for(unsigned i=0;i<GRID_SCALAR_SLOTS;i++){
  GridScalarFlag *f=s->flags+i;
  if(f->epoch==s->epoch&&f->cap==cap){f->value=enabled!=0;return 1;}
  if((f->epoch!=s->epoch||!f->cap)&&free_slot==GRID_SCALAR_SLOTS)free_slot=i;
 }
 if(free_slot==GRID_SCALAR_SLOTS){s->valid=0;return 0;}
 s->flags[free_slot]=(GridScalarFlag){cap,enabled!=0,s->epoch};s->used++;return 1;
}
void grid_scalar_blend(GridScalarState *s,uint32_t src,uint32_t dst){s->src=src;s->dst=dst;s->blend_epoch=s->epoch;}
void grid_scalar_depth(GridScalarState *s,int enabled){s->depth_write=enabled!=0;s->depth_epoch=s->epoch;}
int grid_scalar_equal(const GridScalarState *a,const GridScalarState *b){
 if(!a->valid||!b->valid||a->epoch!=b->epoch||a->used!=b->used)return 0;
 int ax=a->blend_epoch==a->epoch,by=b->blend_epoch==b->epoch;
 if(ax!=by||(ax&&(a->src!=b->src||a->dst!=b->dst)))return 0;
 ax=a->depth_epoch==a->epoch;by=b->depth_epoch==b->epoch;
 if(ax!=by||(ax&&a->depth_write!=b->depth_write))return 0;
 for(unsigned i=0;i<GRID_SCALAR_SLOTS;i++){
  const GridScalarFlag *x=a->flags+i,*y=b->flags+i;
  ax=x->cap&&x->epoch==a->epoch;by=y->cap&&y->epoch==b->epoch;
  if(ax!=by||(ax&&memcmp(x,y,sizeof *x)))return 0;
 }
 return 1;
}
