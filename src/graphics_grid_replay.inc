/* Frame-local GPU replay. Caller additionally owns the managed-state contract.
 * Preflight examines the complete stream before any graphics mutation. */
typedef struct {unsigned mask,draws;int valid;} GridReplayCheck;
static void grid_replay_check(void *context,GridCommandOp op,const uint32_t a[4],const void *payload,size_t size){
 (void)payload;(void)size;GridReplayCheck *c=context;
 switch(op){
 case GRID_CMD_ATTR_ENABLE:if(a[0]>=3)c->valid=0;else c->mask|=1u<<a[0];break;
 case GRID_CMD_ATTR_DISABLE:if(a[0]>=3)c->valid=0;else c->mask&=~(1u<<a[0]);break;
 case GRID_CMD_TRIANGLES:if(c->mask!=7||a[0]!=0||a[1]!=1||a[2]!=2)c->valid=0;c->draws++;break;
 case GRID_CMD_ACTIVE_TEXTURE:if(a[0]!=GL_TEXTURE0)c->valid=0;break;
 case GRID_CMD_TEXTURE:if(a[0]!=GL_TEXTURE_2D)c->valid=0;break;
 default:break;
 }
}
int graphics_grid_replay_ready(unsigned frame){
 if(grid_stream_pass!=2||grid_stream_frame!=frame||!graphics_guard_ready()||!grid_uniform_incoming_equal||!grid_scalar_incoming_equal||gt_outside_mask||gt_unknown_index)return 0;
 for(unsigned i=0;i<GT_ATTR;i++)if(gt_attributes[i].enabled)return 0;
 GLint buffer=0,element=0,texture=0;glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&element);glGetIntegerv(GL_ACTIVE_TEXTURE,&texture);
 if(buffer||element||texture!=GL_TEXTURE0)return 0;
 GridReplayCheck check={.valid=1};
 return grid_commands_replay(grid_streams,frame,grid_replay_check,&check)&&check.valid&&check.draws&&!check.mask;
}
static void grid_replay_command(void *context,GridCommandOp op,const uint32_t a[4],const void *payload,size_t size){
 (void)context;(void)size;
 switch(op){
 case GRID_CMD_PROGRAM:timed_program(a[0]);break;
 case GRID_CMD_TEXTURE:timed_bind_texture(a[0],a[1]);break;
 case GRID_CMD_ACTIVE_TEXTURE:stream_active_texture(a[0]);break;
 case GRID_CMD_ENABLE:stream_enable(a[0]);break;
 case GRID_CMD_DISABLE:stream_disable(a[0]);break;
 case GRID_CMD_BLEND:stream_blend(a[0],a[1]);break;
 case GRID_CMD_DEPTH_MASK:stream_depth_mask((GLboolean)a[0]);break;
 case GRID_CMD_UNIFORM_I:stream_uniform_i((GLint)a[0],(GLint)a[1]);break;
 case GRID_CMD_MATRIX4:timed_matrix((GLint)a[0],(GLsizei)a[1],(GLboolean)a[2],payload);break;
 case GRID_CMD_ATTR_ENABLE:gt_enable(a[0]);break;
 case GRID_CMD_ATTR_DISABLE:gt_disable(a[0]);break;
 case GRID_CMD_TRIANGLES:
  timed_attrib(0,2,GL_FLOAT,0,20,payload);
  timed_attrib(1,4,GL_UNSIGNED_BYTE,1,20,(const unsigned char*)payload+8);
  timed_attrib(2,2,GL_FLOAT,0,20,(const unsigned char*)payload+12);
  bridge_draw_arrays(GL_TRIANGLES,0,(GLsizei)a[3]);break;
 }
}
int graphics_grid_replay(unsigned frame){
 if(!graphics_grid_replay_ready(frame))return 0;
 grid_stream_active=NULL;gt_sample=0;
 int result=grid_commands_replay(grid_streams,frame,grid_replay_command,NULL);
 grid_commands_invalidate(grid_streams);return result;
}
