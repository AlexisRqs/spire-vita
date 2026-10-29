/* Draw-time geometry comparison and input validation for command capture.
 * Client payload is copied by the recorder; unsupported shapes reject reuse. */
#define GT_ATTR 8
#define GT_DRAWS 512
typedef struct {GLint size;GLenum type;GLboolean normalized;GLsizei stride;const unsigned char *pointer;int enabled;GLuint buffer;} GTAttrib;
typedef struct {uint64_t geometry[GT_ATTR],state;unsigned mask,valid;} GTPacket;
static GTAttrib gt_attributes[GT_ATTR];
static GTPacket gt_first[GT_DRAWS];
static unsigned gt_pass,gt_count,gt_first_count,gt_invalid,gt_mismatch,gt_state_mismatch,gt_mask_mismatch,gt_attr_mismatch[GT_ATTR];
static int gt_sample,gt_report,gt_first_difference;
static unsigned gt_pointer_mask,gt_outside_mask;static int gt_unknown_index;
static uint64_t gt_hash(uint64_t h,const void *p,size_t n){const unsigned char *b=p;for(size_t i=0;i<n;i++)h=(h^b[i])*1099511628211ull;return h;}
static void gt_attrib(GLuint index,GLint size,GLenum type,GLboolean normalized,GLsizei stride,const void *pointer){
 if(!gt_sample||index>=GT_ATTR)return;
 GLint buffer=0;glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);
 gt_pointer_mask|=1u<<index;
 GTAttrib *a=gt_attributes+index;int enabled=a->enabled;
 *a=(GTAttrib){size,type,normalized,stride,pointer,enabled,(GLuint)buffer};
}
static void gt_enable(GLuint index){grid_stream_event(GRID_CMD_ATTR_ENABLE,index,0,0,0,NULL,0);if(index<GT_ATTR)gt_attributes[index].enabled=1;else if(index<32)gt_outside_mask|=1u<<index;else gt_unknown_index=1;glEnableVertexAttribArray(index);}
static void gt_disable(GLuint index){grid_stream_event(GRID_CMD_ATTR_DISABLE,index,0,0,0,NULL,0);if(index<GT_ATTR)gt_attributes[index].enabled=0;else if(index<32)gt_outside_mask&=~(1u<<index);glDisableVertexAttribArray(index);}
void graphics_grid_trace_begin(unsigned pass,int sample){
 gt_first_difference=-1;gt_pointer_mask=0;gt_report=sample==1;gt_sample=sample&&graphics_cpu_trace_available();gt_pass=pass;gt_count=0;gt_invalid=gt_mismatch=gt_state_mismatch=gt_mask_mismatch=0;memset(gt_attr_mismatch,0,sizeof gt_attr_mismatch);
}
void graphics_grid_trace_end(void){
 if(gt_sample&&gt_pass==1)gt_first_count=gt_count;
 if(gt_sample&&gt_report&&gt_pass==2)android_log_print(4,"GridTrace","first=%u second=%u invalid=%u different=%u first_difference=%d state=%u mask=%u attr0=%u attr1=%u attr2=%u attr3=%u",gt_first_count,gt_count,gt_invalid,gt_mismatch,gt_first_difference,gt_state_mismatch,gt_mask_mismatch,gt_attr_mismatch[0],gt_attr_mismatch[1],gt_attr_mismatch[2],gt_attr_mismatch[3]);
 gt_sample=0;
}
static void gt_draw(GLenum mode,GLsizei count,GLenum type,const void *indices,int first){
 if(!gt_sample)return;
 unsigned at=gt_count++;GTPacket packet={0};
 GLint element=0;glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&element);
 if(gt_outside_mask||gt_unknown_index||at>=GT_DRAWS||count<0||count>8192||first<0||(!type&&(first>=32768||count>32768-first))||(type&&(element||!indices||type!=GL_UNSIGNED_SHORT)))goto reject;
 GLint values[9];const GLenum keys[]={GL_CURRENT_PROGRAM,GL_TEXTURE_BINDING_2D,GL_BLEND_SRC_RGB,GL_BLEND_DST_RGB,GL_BLEND_SRC_ALPHA,GL_BLEND_DST_ALPHA,GL_BLEND_EQUATION_RGB,GL_BLEND_EQUATION_ALPHA,GL_ACTIVE_TEXTURE};
 for(unsigned i=0;i<9;i++)glGetIntegerv(keys[i],values+i);
 packet.state=gt_hash(1469598103934665603ull,values,sizeof values);packet.state=gt_hash(packet.state,&mode,sizeof mode);packet.state=gt_hash(packet.state,&count,sizeof count);
 for(unsigned i=0;i<GT_ATTR;i++){
  GTAttrib *a=gt_attributes+i;if(!a->enabled)continue;
  packet.mask|=1u<<i;
  unsigned component=a->type==GL_FLOAT?4:a->type==GL_UNSIGNED_BYTE?1:0;
  if(!(gt_pointer_mask&(1u<<i))||a->buffer||!a->pointer||a->size<1||a->size>4||!component||a->stride<0||a->stride>256)goto reject;
  unsigned width=component*(unsigned)a->size,stride=a->stride?(unsigned)a->stride:width;
  uint64_t h=gt_hash(1469598103934665603ull,&a->type,sizeof a->type);h=gt_hash(h,&a->size,sizeof a->size);h=gt_hash(h,&a->normalized,sizeof a->normalized);
  for(int j=0;j<count;j++){
   unsigned index=indices?((const uint16_t*)indices)[j]:(unsigned)(first+j);
   if(index>=32768)goto reject;
   h=gt_hash(h,a->pointer+(size_t)index*stride,width);
  }
  packet.geometry[i]=h;
 }
 packet.valid=1;
 if(grid_stream_active&&grid_stream_active->valid){
  static unsigned char packed[8190*20];
  GTAttrib *p=gt_attributes,*c=p+1,*uv=p+2;
  if(values[8]!=GL_TEXTURE0||!grid_stream_program_supported((GLuint)values[0])||mode!=GL_TRIANGLES||count<=0||count>8190||count%3||packet.mask!=7||
     p->type!=GL_FLOAT||p->size!=2||p->normalized||c->type!=GL_UNSIGNED_BYTE||c->size!=4||!c->normalized||uv->type!=GL_FLOAT||uv->size!=2||uv->normalized)grid_stream_reject();
  else{
   for(int j=0;j<count;j++){
    unsigned ix=indices?((const uint16_t*)indices)[j]:(unsigned)(first+j);
    memcpy(packed+j*20,p->pointer+(size_t)ix*(p->stride?p->stride:8),8);
    memcpy(packed+j*20+8,c->pointer+(size_t)ix*(c->stride?c->stride:4),4);
    memcpy(packed+j*20+12,uv->pointer+(size_t)ix*(uv->stride?uv->stride:8),8);
   }
   /* Explicit per-draw bindings make replay independent of incoming program
    * and unit0 texture; the supported shader has exactly one sampler. */
   grid_stream_event(GRID_CMD_PROGRAM,(unsigned)values[0],0,0,0,NULL,0);
   grid_stream_event(GRID_CMD_ACTIVE_TEXTURE,GL_TEXTURE0,0,0,0,NULL,0);
   grid_stream_event(GRID_CMD_TEXTURE,GL_TEXTURE_2D,(unsigned)values[1],0,0,NULL,0);
   grid_stream_event(GRID_CMD_TRIANGLES,0,1,2,(unsigned)count,packed,(size_t)count*20);
  }
 }

 reject:
 if(!packet.valid){gt_invalid++;grid_stream_reject();}
 if(gt_pass==1) {if(at<GT_DRAWS)gt_first[at]=packet;return;}
 if(gt_pass!=2)return;
 if(at>=GT_DRAWS||at>=gt_first_count){gt_mismatch++;return;}
 GTPacket *old=gt_first+at;
 if(!old->valid||!packet.valid){gt_mismatch++;return;}
 int different=packet.state!=old->state||packet.mask!=old->mask;
 if(packet.state!=old->state)gt_state_mismatch++;
 if(packet.mask!=old->mask)gt_mask_mismatch++;
 for(unsigned i=0;i<GT_ATTR;i++)if(packet.geometry[i]!=old->geometry[i]){gt_attr_mismatch[i]++;different=1;}
 if(different){if(gt_first_difference<0)gt_first_difference=(int)at;gt_mismatch++;}
}
