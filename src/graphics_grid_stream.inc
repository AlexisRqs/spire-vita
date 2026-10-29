/* Bounded recorder. Mode5 compares both original calls; opt-in mode6 may
 * replay only after independent graphics and managed-state guards pass. */
#include "grid_commands.h"
#include "grid_uniforms.h"
#include "grid_scalar_state.h"
#define GRID_STREAM_CAPACITY (512u*1024u)
_Alignas(16) static unsigned char grid_stream_storage[2][GRID_STREAM_CAPACITY];
static GridCommands grid_streams[2],*grid_stream_active;
static int grid_stream_report;
static unsigned grid_stream_pass,grid_stream_frame,grid_stream_unknown;
static GridUniformState grid_uniform_live={.valid=1},grid_uniform_initial;
static int grid_uniform_incoming_equal,grid_scalar_incoming_equal;
typedef struct {GLuint program;GLint matrix,sampler;int supported;} GridProgramShape;
static GridProgramShape grid_program_shapes[16];
static int grid_stream_program_supported(GLuint program){
 if(!grid_stream_active||!grid_stream_active->valid||!program)return 0;
 GridProgramShape *shape=NULL;
 for(unsigned i=0;i<16;i++)if(grid_program_shapes[i].program==program){shape=grid_program_shapes+i;break;}
 if(!shape){
  for(unsigned i=0;i<16;i++)if(!grid_program_shapes[i].program){shape=grid_program_shapes+i;break;}
  if(!shape)return 0;
  *shape=(GridProgramShape){.program=program,.matrix=-1,.sampler=-1};
  GLint count=0;glGetProgramiv(program,GL_ACTIVE_UNIFORMS,&count);
  if(count!=2)return 0;
  for(GLuint i=0;i<2;i++){
   GLchar name[64]={0};GLsizei length=0;GLint size=0;GLenum type=0;
   glGetActiveUniform(program,i,sizeof name,&length,&size,&type,name);
   if(length<=0||length>=63||size!=1)return 0;
   if(type==GL_FLOAT_MAT4&&!strcmp(name,"u_projTrans"))shape->matrix=glGetUniformLocation(program,name);
   else if(type==GL_SAMPLER_2D&&!strcmp(name,"u_texture"))shape->sampler=glGetUniformLocation(program,name);
   else return 0;
  }
  shape->supported=shape->matrix>=0&&shape->sampler>=0&&shape->matrix!=shape->sampler;
 }
 return shape->supported&&grid_uniforms_sprite_program(&grid_uniform_live,program,shape->matrix,shape->sampler);
}
static GridScalarState grid_scalar_live={.epoch=1,.valid=1},grid_scalar_initial;
static void grid_uniform_matrix(GLint location,GLsizei count,GLboolean transpose,const void *value){
 if(!graphics_guard_ready())return;
 GLint program=0;glGetIntegerv(GL_CURRENT_PROGRAM,&program);
 grid_uniforms_matrix(&grid_uniform_live,(unsigned)program,location,count,transpose,value);
}
void graphics_grid_unknown_call(const char *name){
 grid_scalar_reset(&grid_scalar_live);
 /* Uncovered uniform operations or program replacement erase our knowledge.
  * Existing captures are invalidated below; a reset epoch cannot compare equal. */
 if(strstr(name,"Uniform")||strstr(name,"Program"))grid_uniforms_reset(&grid_uniform_live);
 if(strstr(name,"Program"))memset(grid_program_shapes,0,sizeof grid_program_shapes);
 if(!grid_stream_active&&grid_streams[0].valid&&grid_streams[0].sealed)android_log_print(4,"GridStreamUnsupported","between_passes=1 name=%s",name);
 if(grid_stream_active){if(!grid_stream_unknown)android_log_print(4,"GridStreamUnsupported","name=%s",name);grid_stream_unknown++;}
 grid_commands_invalidate(grid_streams);grid_commands_invalidate(grid_streams+1);
}
static void grid_stream_event(GridCommandOp op,uint32_t a,uint32_t b,uint32_t c,uint32_t d,const void *payload,size_t size){
 if(grid_stream_active){uint32_t args[4]={a,b,c,d};grid_commands_append(grid_stream_active,op,args,payload,size);}
}
static void grid_stream_reject(void){if(grid_stream_active)grid_commands_invalidate(grid_stream_active);}
void graphics_grid_stream_begin(unsigned pass,unsigned frame,int sample){
 grid_stream_active=NULL;grid_stream_report=sample==1;grid_stream_pass=pass;grid_stream_frame=frame;
 if(!sample||pass<1||pass>2||!graphics_cpu_trace_available())return;
 if(!graphics_guard_ready()){android_log_print(4,"GridStream","guard_unavailable=1");return;}
 grid_stream_unknown=0;
 if(pass==1){grid_uniform_initial=grid_uniform_live;grid_scalar_initial=grid_scalar_live;grid_uniform_incoming_equal=grid_scalar_incoming_equal=0;}
 else{grid_uniform_incoming_equal=grid_uniforms_equal(&grid_uniform_live,&grid_uniform_initial);grid_scalar_incoming_equal=grid_scalar_equal(&grid_scalar_live,&grid_scalar_initial);}
 GridCommands *s=&grid_streams[pass-1];grid_commands_init(s,grid_stream_storage[pass-1],GRID_STREAM_CAPACITY);grid_commands_begin(s,frame);grid_stream_active=s;
 GLint active=0;glGetIntegerv(GL_ACTIVE_TEXTURE,&active);if(active!=GL_TEXTURE0)grid_stream_reject();
}
void graphics_grid_stream_end(void){
 if(!grid_stream_active)return;
 grid_commands_seal(grid_stream_active);
 if(grid_stream_pass==2&&grid_stream_report)android_log_print(4,"GridStream","frame=%u first_valid=%u second_valid=%u equal=%d first_events=%u second_events=%u bytes=%u unknown=%u incoming_uniforms_equal=%d incoming_scalar_equal=%d managed_state_unverified=1",grid_stream_frame,grid_streams[0].valid,grid_streams[1].valid,grid_commands_equal(grid_streams,grid_streams+1,grid_stream_frame),grid_streams[0].count,grid_streams[1].count,(unsigned)grid_streams[0].used,grid_stream_unknown,grid_uniform_incoming_equal,grid_scalar_incoming_equal);
 grid_stream_active=NULL;
}
static void stream_enable(GLenum cap){if(graphics_guard_ready())grid_scalar_enable(&grid_scalar_live,cap,1);grid_stream_event(GRID_CMD_ENABLE,cap,0,0,0,NULL,0);glEnable(cap);}
static void stream_disable(GLenum cap){if(graphics_guard_ready())grid_scalar_enable(&grid_scalar_live,cap,0);grid_stream_event(GRID_CMD_DISABLE,cap,0,0,0,NULL,0);glDisable(cap);}
static void stream_blend(GLenum src,GLenum dst){if(graphics_guard_ready())grid_scalar_blend(&grid_scalar_live,src,dst);grid_stream_event(GRID_CMD_BLEND,src,dst,0,0,NULL,0);glBlendFunc(src,dst);}
static void stream_active_texture(GLenum texture){if(texture!=GL_TEXTURE0)grid_stream_reject();grid_stream_event(GRID_CMD_ACTIVE_TEXTURE,texture,0,0,0,NULL,0);glActiveTexture(texture);}
static void stream_uniform_i(GLint location,GLint value){if(graphics_guard_ready()){GLint program=0;glGetIntegerv(GL_CURRENT_PROGRAM,&program);grid_uniforms_integer(&grid_uniform_live,(unsigned)program,location,value);}grid_stream_event(GRID_CMD_UNIFORM_I,(uint32_t)location,(uint32_t)value,0,0,NULL,0);glUniform1i(location,value);}

static void stream_depth_mask(GLboolean flag){if(graphics_guard_ready())grid_scalar_depth(&grid_scalar_live,flag);grid_stream_event(GRID_CMD_DEPTH_MASK,flag!=0,0,0,0,NULL,0);glDepthMask(flag);}
