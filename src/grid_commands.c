#include "grid_commands.h"
#include <string.h>
#include <limits.h>
typedef struct {uint32_t op,size,args[4];} Command;
static int shape(const Command *c){
 switch(c->op){
 case GRID_CMD_DEPTH_MASK:
  return !c->size&&c->args[0]<=1&&!c->args[1]&&!c->args[2]&&!c->args[3];
 case GRID_CMD_PROGRAM:case GRID_CMD_ACTIVE_TEXTURE:case GRID_CMD_ENABLE:case GRID_CMD_DISABLE:
 case GRID_CMD_ATTR_ENABLE:case GRID_CMD_ATTR_DISABLE:
  return !c->size&&!c->args[1]&&!c->args[2]&&!c->args[3];
 case GRID_CMD_TEXTURE:case GRID_CMD_BLEND:case GRID_CMD_UNIFORM_I:
  return !c->size&&!c->args[2]&&!c->args[3];
 case GRID_CMD_MATRIX4:
  return c->args[1]>0&&c->args[1]<=4&&c->args[2]<=1&&!c->args[3]&&c->size==c->args[1]*64;
 case GRID_CMD_TRIANGLES:
  /* Expanded indexed triangles, XY + packed RGBA + UV (20 bytes/vertex).
   * Attribute locations are distinct; the GL adapter validates their formats. */
  return c->args[0]<8&&c->args[1]<8&&c->args[2]<8&&c->args[0]!=c->args[1]&&c->args[0]!=c->args[2]&&c->args[1]!=c->args[2]&&c->args[3]>0&&c->args[3]<=8190&&c->args[3]%3==0&&c->size==c->args[3]*20;
 default:return 0;
 }
}
void grid_commands_init(GridCommands *s,void *storage,size_t capacity){*s=(GridCommands){.bytes=storage,.capacity=capacity};}
void grid_commands_begin(GridCommands *s,uint32_t frame){s->used=s->count=0;s->frame=frame;s->recording=1;s->sealed=0;s->valid=s->bytes&&s->capacity>=sizeof(Command);}
void grid_commands_invalidate(GridCommands *s){s->valid=0;s->sealed=0;s->recording=0;}
int grid_commands_append(GridCommands *s,GridCommandOp op,const uint32_t args[4],const void *payload,size_t size){
 if(!s->recording||!s->valid)return 0;
 if(!args||size>UINT32_MAX||(!payload&&size)){grid_commands_invalidate(s);return 0;}
 Command c={.op=op,.size=(uint32_t)size};memcpy(c.args,args,sizeof c.args);
 if(!shape(&c)||s->used>s->capacity||sizeof c>s->capacity-s->used||size>s->capacity-s->used-sizeof c||s->count==UINT32_MAX){grid_commands_invalidate(s);return 0;}
 memcpy(s->bytes+s->used,&c,sizeof c);if(size)memcpy(s->bytes+s->used+sizeof c,payload,size);
 s->used+=sizeof c+size;s->count++;return 1;
}
static int check(const GridCommands *s,uint32_t frame){
 if(!s->valid||!s->sealed||s->recording||s->frame!=frame||!s->bytes||!s->count||s->used>s->capacity)return 0;
 size_t at=0;uint32_t count=0;
 while(at<s->used){
  if(s->used-at<sizeof(Command))return 0;
  Command c;memcpy(&c,s->bytes+at,sizeof c);
  if(!shape(&c)||c.size>s->used-at-sizeof c)return 0;
  at+=sizeof c+c.size;count++;
 }
 return count==s->count;
}
int grid_commands_seal(GridCommands *s){s->recording=0;s->sealed=s->valid;if(!check(s,s->frame)){grid_commands_invalidate(s);return 0;}return 1;}
int grid_commands_equal(const GridCommands *a,const GridCommands *b,uint32_t frame){return check(a,frame)&&check(b,frame)&&a->count==b->count&&a->used==b->used&&!memcmp(a->bytes,b->bytes,a->used);}
int grid_commands_replay(const GridCommands *s,uint32_t frame,GridCommandSink sink,void *context){
 if(!sink||!check(s,frame))return 0;
 size_t at=0;
 while(at<s->used){Command c;memcpy(&c,s->bytes+at,sizeof c);sink(context,(GridCommandOp)c.op,c.args,s->bytes+at+sizeof c,c.size);at+=sizeof c+c.size;}
 return 1;
}
