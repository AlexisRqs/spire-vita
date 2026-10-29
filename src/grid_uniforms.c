#include "grid_uniforms.h"
#include <string.h>
void grid_uniforms_reset(GridUniformState *s){
 /* Unknown setters may occur every frame; invalidate in O(1), not a 70KiB clear. */
 if(++s->epoch==0){memset(s->entries,0,sizeof s->entries);s->epoch=1;}
 s->used=0;s->valid=1;
}
static int set(GridUniformState *s,uint32_t program,int32_t location,unsigned kind,unsigned count,const void *value,unsigned bytes){
 if(!s->valid)return 0;
 if(location==-1)return 1; /* GL ignores location -1; no state changes. */
 if(location<0||!program||!value||bytes>256){s->valid=0;return 0;}
 unsigned hash=(program*2654435761u^(uint32_t)location*2246822519u)&(GRID_UNIFORM_SLOTS-1);
 for(unsigned probe=0;probe<GRID_UNIFORM_SLOTS;probe++){
  GridUniform *e=&s->entries[(hash+probe)&(GRID_UNIFORM_SLOTS-1)];
  if(e->epoch!=s->epoch||!e->program||(e->program==program&&e->location==location)){
   if(e->epoch==s->epoch&&e->program&&e->kind!=kind){s->valid=0;return 0;}
   if(e->epoch!=s->epoch||!e->program)s->used++;
   memset(e,0,sizeof *e);e->program=program;e->location=location;e->kind=kind;e->count=count;e->epoch=s->epoch;memcpy(e->value,value,bytes);return 1;
  }
 }
 s->valid=0;return 0;
}
int grid_uniforms_integer(GridUniformState *s,uint32_t program,int32_t location,int32_t value){return set(s,program,location,1,1,&value,4);}
int grid_uniforms_matrix(GridUniformState *s,uint32_t program,int32_t location,int count,int transpose,const void *value){
 if(location==-1)return s->valid;
 if(count<=0||count>4||transpose){s->valid=0;return 0;}
 return set(s,program,location,2,(unsigned)count,value,(unsigned)count*64);
}
int grid_uniforms_equal(const GridUniformState *a,const GridUniformState *b){
 if(!a->valid||!b->valid||a->epoch!=b->epoch||a->used!=b->used)return 0;
 for(unsigned i=0;i<GRID_UNIFORM_SLOTS;i++){
  const GridUniform *x=a->entries+i,*y=b->entries+i;
  int ax=x->program&&x->epoch==a->epoch,by=y->program&&y->epoch==b->epoch;
  if(ax!=by||(ax&&memcmp(x,y,sizeof *x)))return 0;
 }
 return 1;
}

int grid_uniforms_sprite_program(const GridUniformState *s,uint32_t program,int matrix_location,int sampler_location){
 if(!s->valid||!program||matrix_location<0||sampler_location<0||matrix_location==sampler_location)return 0;
 int matrix=0,sampler=0;
 for(unsigned i=0;i<GRID_UNIFORM_SLOTS;i++){
  const GridUniform *e=s->entries+i;if(e->epoch!=s->epoch||e->program!=program)continue;
  if(e->location==matrix_location)matrix=e->kind==2&&e->count==1;
  if(e->location==sampler_location){int32_t value;memcpy(&value,e->value,4);sampler=e->kind==1&&e->count==1&&value==0;}
 }
 return matrix&&sampler;
}
