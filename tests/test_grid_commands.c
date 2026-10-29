#include "grid_commands.h"
#include <assert.h>
#include <string.h>
static unsigned calls;
static void sink(void *p,GridCommandOp op,const uint32_t args[4],const void *payload,size_t size){GridCommands *s=p;calls++;assert(grid_commands_append(s,op,args,payload,size));}
int main(void){
 unsigned char a[4096],b[4096],payload[120],copy[120];for(unsigned i=0;i<sizeof payload;i++)payload[i]=(unsigned char)i;memcpy(copy,payload,sizeof copy);
 GridCommands first,second;grid_commands_init(&first,a,sizeof a);grid_commands_init(&second,b,sizeof b);
 grid_commands_begin(&first,17);uint32_t program[4]={9},matrix[4]={3,1,0,0},draw[4]={0,1,2,6};
 assert(grid_commands_append(&first,GRID_CMD_PROGRAM,program,NULL,0));assert(grid_commands_append(&first,GRID_CMD_MATRIX4,matrix,payload,64));assert(grid_commands_append(&first,GRID_CMD_TRIANGLES,draw,payload,sizeof payload));assert(grid_commands_seal(&first));
 memset(payload,255,sizeof payload);grid_commands_begin(&second,17);assert(grid_commands_replay(&first,17,sink,&second)&&calls==3);assert(grid_commands_seal(&second));assert(grid_commands_equal(&first,&second,17));
 assert(!grid_commands_replay(&first,18,sink,&second)&&calls==3);assert(!grid_commands_equal(&first,&second,18));
 /* Payload is copied; mutable original memory cannot change the stored draw. */
 assert(!memcmp(first.bytes+first.used-120,copy,120));
 /* Entire malformed stream is rejected before forwarding even its first op. */
 uint32_t bad=999;memcpy(first.bytes+first.used-120-24,&bad,4);calls=0;assert(!grid_commands_replay(&first,17,sink,&second)&&!calls);
 grid_commands_begin(&first,19);assert(!grid_commands_append(&first,GRID_CMD_MATRIX4,matrix,NULL,64));assert(!grid_commands_seal(&first));
 grid_commands_begin(&first,20);assert(!grid_commands_append(&first,(GridCommandOp)999,program,NULL,0));assert(!first.valid);
 grid_commands_init(&first,a,24);grid_commands_begin(&first,21);assert(grid_commands_append(&first,GRID_CMD_PROGRAM,program,NULL,0));assert(!grid_commands_append(&first,GRID_CMD_PROGRAM,program,NULL,0));assert(!grid_commands_seal(&first));
 grid_commands_init(&first,a,sizeof a);grid_commands_begin(&first,22);draw[3]=5;assert(!grid_commands_append(&first,GRID_CMD_TRIANGLES,draw,copy,100));
 grid_commands_begin(&first,23);assert(!grid_commands_seal(&first));
 grid_commands_begin(&first,24);assert(grid_commands_append(&first,GRID_CMD_PROGRAM,program,NULL,0));assert(grid_commands_seal(&first));grid_commands_invalidate(&first);assert(!grid_commands_replay(&first,24,sink,&second));
 grid_commands_begin(&first,25);uint32_t depth[4]={1};assert(grid_commands_append(&first,GRID_CMD_DEPTH_MASK,depth,NULL,0));assert(grid_commands_seal(&first));grid_commands_begin(&second,25);calls=0;assert(grid_commands_replay(&first,25,sink,&second)&&calls==1);assert(grid_commands_seal(&second)&&grid_commands_equal(&first,&second,25));
 grid_commands_begin(&first,26);depth[0]=2;assert(!grid_commands_append(&first,GRID_CMD_DEPTH_MASK,depth,NULL,0));
 return 0;
}
