/* Opt-in timings of GL work outside draw submission. Same arguments forwarded. */
static int cpu_profile=-1;
static uint64_t buffer_us,attrib_us,uniform_us,texture_state_us,program_us,buffer_bytes;
static unsigned buffer_calls,attrib_calls,uniform_calls,texture_state_calls,program_calls;
static void timed_buffer_data(GLenum target,GLsizeiptr size,const void *data,GLenum usage){
 uint64_t t=sceKernelGetProcessTimeWide();glBufferData(target,size,data,usage);buffer_us+=sceKernelGetProcessTimeWide()-t;buffer_calls++;if(size>0)buffer_bytes+=(uint64_t)size;
}
static void timed_buffer_sub(GLenum target,GLintptr offset,GLsizeiptr size,const void *data){
 uint64_t t=sceKernelGetProcessTimeWide();glBufferSubData(target,offset,size,data);buffer_us+=sceKernelGetProcessTimeWide()-t;buffer_calls++;if(size>0)buffer_bytes+=(uint64_t)size;
}
static void timed_attrib(GLuint index,GLint size,GLenum type,GLboolean normalized,GLsizei stride,const void *pointer){
 gt_attrib(index,size,type,normalized,stride,pointer);
 uint64_t t=sceKernelGetProcessTimeWide();glVertexAttribPointer(index,size,type,normalized,stride,pointer);attrib_us+=sceKernelGetProcessTimeWide()-t;attrib_calls++;
}
static void timed_matrix(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value){
 grid_uniform_matrix(location,count,transpose,value);
 if(count>0&&count<=4)grid_stream_event(GRID_CMD_MATRIX4,(uint32_t)location,(uint32_t)count,transpose,0,value,(size_t)count*64);else grid_stream_reject();
 uint64_t t=sceKernelGetProcessTimeWide();glUniformMatrix4fv(location,count,transpose,value);uniform_us+=sceKernelGetProcessTimeWide()-t;uniform_calls++;
}
static void timed_bind_texture(GLenum target,GLuint texture){
 if(target!=GL_TEXTURE_2D)grid_stream_reject();
 grid_stream_event(GRID_CMD_TEXTURE,target,texture,0,0,NULL,0);
 uint64_t t=sceKernelGetProcessTimeWide();glBindTexture(target,texture);texture_state_us+=sceKernelGetProcessTimeWide()-t;texture_state_calls++;
}
static void timed_program(GLuint program){
 grid_stream_event(GRID_CMD_PROGRAM,program,0,0,0,NULL,0);
 uint64_t t=sceKernelGetProcessTimeWide();glUseProgram(program);program_us+=sceKernelGetProcessTimeWide()-t;program_calls++;
}
static uintptr_t graphics_cpu_lookup(const char *name){
 if(cpu_profile<0){FILE *f=fopen("ux0:data/spireandroid/enable-gl-profile.flag","rb");cpu_profile=f!=NULL;if(f)fclose(f);}
 if(!cpu_profile)return 0;
 if(!strcmp(name,"glBufferData"))return (uintptr_t)timed_buffer_data;
 if(!strcmp(name,"glBufferSubData"))return (uintptr_t)timed_buffer_sub;
 if(!strcmp(name,"glVertexAttribPointer"))return (uintptr_t)timed_attrib;
 if(!strcmp(name,"glUniformMatrix4fv"))return (uintptr_t)timed_matrix;
 if(!strcmp(name,"glBindTexture"))return (uintptr_t)timed_bind_texture;
 if(!strcmp(name,"glUseProgram"))return (uintptr_t)timed_program;
 return 0;
}
static void graphics_cpu_reset(void){buffer_us=attrib_us=uniform_us=texture_state_us=program_us=buffer_bytes=0;buffer_calls=attrib_calls=uniform_calls=texture_state_calls=program_calls=0;}
static void graphics_cpu_report(void){
 if(cpu_profile>0)android_log_print(4,"GLCost","buffer_us=%llu attrib_us=%llu uniform_us=%llu texture_us=%llu program_us=%llu buffer_bytes=%llu buffer_calls=%u attrib_calls=%u uniform_calls=%u texture_calls=%u program_calls=%u",(unsigned long long)buffer_us,(unsigned long long)attrib_us,(unsigned long long)uniform_us,(unsigned long long)texture_state_us,(unsigned long long)program_us,(unsigned long long)buffer_bytes,buffer_calls,attrib_calls,uniform_calls,texture_state_calls,program_calls);
}
