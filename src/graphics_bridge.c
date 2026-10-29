#include "graphics_bridge.h"
#include "graphics_guard.h"
#include "frame_distribution.h"
#include "launcher_settings.h"
#include "launcher_background.h"
#include "debugScreen.h"
#include "description_strings.h"
void performance_clocks_report(void);
#include "compat_libc.h"
#include "android_log.h"
#include "keyboard.h"
#include "loading_screen.h"
#include "scene_metrics.h"
#include "direct_cache.h"
#include "text_fast.h"
#include "text_submit.h"
#include "buffer_pointer.h"
#include "glyph_fast.h"
#include "card_profile.h"
#include "gameplay_profile.h"
#include "text_memo.h"
#include "map_cull.h"
#include "vita_frame.h"
#include <psp2/kernel/processmgr.h>
#include "android_assets.h"
#include <vitaGL.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include "graphics_float.inc"
#include "graphics_etc2.inc"
#include "graphics_cursor.inc"
static int graphics_cpu_trace_available(void);
#include "graphics_grid_stream.inc"
#include "graphics_grid_trace.inc"
#include "graphics_cpu_profile.inc"
static int graphics_cpu_trace_available(void){return cpu_profile>0;}
static uint64_t texture_update_us;
static uint64_t draw_submit_us;
static unsigned texture_update_count,draw_calls;
static void bridge_texture_update(GLenum target,GLint level,GLint x,GLint y,GLsizei w,GLsizei h,GLenum format,GLenum type,const void *pixels){
 uint64_t start=sceKernelGetProcessTimeWide();glTexSubImage2D(target,level,x,y,w,h,format,type,pixels);
 texture_update_us+=sceKernelGetProcessTimeWide()-start;texture_update_count++;
}
static void bridge_draw_elements(GLenum mode,GLsizei count,GLenum type,const void *indices){gt_draw(mode,count,type,indices,0);draw_calls++;uint64_t start=sceKernelGetProcessTimeWide();glDrawElements(mode,count,type,indices);draw_submit_us+=sceKernelGetProcessTimeWide()-start;}
static void bridge_draw_arrays(GLenum mode,GLint first,GLsizei count){gt_draw(mode,count,0,NULL,first);draw_calls++;uint64_t start=sceKernelGetProcessTimeWide();glDrawArrays(mode,first,count);draw_submit_us+=sceKernelGetProcessTimeWide()-start;}
#include "graphics_grid_replay.inc"
static void graphics_profile_reset(void){graphics_cpu_reset();texture_convert_us=texture_upload_us=texture_bytes=texture_update_us=draw_submit_us=0;texture_upload_count=texture_update_count=draw_calls=texture_cache_hits=0;}
static atomic_uint observed_frames;
static int observe_game,remote_capture,captures_disabled;
void graphics_observe_game(int enabled){
 FILE *disabled=fopen("ux0:data/spireandroid/disable-frame-capture.flag","rb");
 captures_disabled=disabled!=NULL;if(disabled)fclose(disabled);
 FILE *capture_flag=enabled?fopen("ux0:data/spireandroid/enable-remote-capture.flag","rb"):NULL;
 remote_capture=!captures_disabled&&capture_flag!=NULL;if(capture_flag)fclose(capture_flag);
 observe_game=enabled;atomic_store(&observed_frames,0);graphics_profile_reset();(void)texture_cache_profile_take();}
unsigned graphics_game_frames(void){return atomic_load(&observed_frames);}
static void launcher_display_overlay(void *pixels){
 if(launcher_background_active()){
  PsvDebugScreenFont *font=psvDebugScreenGetFont();
  if(font)launcher_background_draw(pixels,font->glyphs);
 }
}
static int bridge_swap(void *display,void *surface){
 (void)display;(void)surface;
 unsigned frame=observe_game?atomic_fetch_add(&observed_frames,1)+1:0;
 if(observe_game&&launcher_background_needs_frame()){
  /* Only advertise Ready after the final menu frame has finished rendering. */
  glFinish();launcher_background_frame_ready();
 }
 if(frame==1&&!launcher_background_active())boot_display_stop();
 int requested=0;
 if(remote_capture&&frame&&frame%120==0){
  FILE *request=fopen("ux0:data/spireandroid/capture-request.flag","rb");
  if(request){fclose(request);requested=remove("ux0:data/spireandroid/capture-request.flag")==0;}
 }
 if(!captures_disabled&&(frame==1||frame==120||requested)){
  GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);glFinish();
  unsigned lit=0;unsigned char *pixels=malloc(960*544*4);
  if(pixels){
   glReadPixels(0,0,960,544,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
   GLenum error=glGetError();
   if(!error){
    char path[96];
    if(requested)strcpy(path,"ux0:data/spireandroid/remote-frame.rgba");
    else snprintf(path,sizeof(path),"ux0:data/spireandroid/frame-%u.rgba",frame);
    FILE *f=fopen(path,"wb");size_t written=f?fwrite(pixels,1,960*544*4,f):0;if(f)fclose(f);
    if(requested&&written==960*544*4){
     FILE *ready=fopen("ux0:data/spireandroid/remote-frame.ready","wb");
     if(ready){fprintf(ready,"%u\n",frame);fclose(ready);}
    }
    for(unsigned i=0;i<960*544;i++)if(pixels[i*4]||pixels[i*4+1]||pixels[i*4+2])lit++;
    android_log_print(4,"Frame","frame=%u viewport=%d,%d,%d,%d nonblack=%u capture_bytes=%u",frame,viewport[0],viewport[1],viewport[2],viewport[3],lit,(unsigned)written);
   }else android_log_print(6,"Frame","Readback frame=%u error=0x%x",frame,error);
   free(pixels);
  }else android_log_print(6,"Frame","Readback allocation failed at frame=%u",frame);
 }
 if(observe_game)graphics_draw_pointer();
 static uint64_t window_start,swap_total,last_present;static unsigned samples;static uint32_t frame_times[120];
 uint64_t before=sceKernelGetProcessTimeWide();
 if(frame==1){window_start=before;last_present=before;swap_total=0;samples=0;}
 vglSwapBuffers(keyboard_active()?GL_TRUE:GL_FALSE);
 uint64_t after=sceKernelGetProcessTimeWide();
 if(observe_game){boot_display_release();scene_present();}
 if(observe_game){frame_times[samples]=(uint32_t)(after-last_present);last_present=after;swap_total+=after-before;samples++;
  if(samples==120){
   if(launcher_settings.diagnostics){
   uint32_t p95,p99,worst;unsigned slow33,slow50;
   frame_distribution(frame_times,samples,&p95,&p99,&worst,&slow33,&slow50);
   android_log_print(4,"FrameDistribution","samples=%u p95_us=%u p99_us=%u max_us=%u over33=%u over50=%u",samples,p95,p99,worst,slow33,slow50);
   performance_clocks_report();
   }
   description_strings_report();card_profile_report();gameplay_profile_report();graphics_cpu_report();glyph_fast_report();text_fast_report();text_submit_report();buffer_pointer_report();text_memo_report();map_cull_report();
   direct_cache_report();
   TextureCacheProfile cp=texture_cache_profile_take();
   android_log_print(4,"CacheProfile","attempts=%u hits=%u source_hash_us=%llu lookup_us=%llu read_us=%llu checksum_us=%llu total_us=%llu wait_us=%llu source_bytes=%llu payload_bytes=%llu",
    cp.attempts,cp.hits,(unsigned long long)cp.source_hash_us,(unsigned long long)cp.lookup_us,(unsigned long long)cp.read_us,(unsigned long long)cp.checksum_us,(unsigned long long)cp.total_us,(unsigned long long)cp.wait_us,(unsigned long long)cp.source_bytes,(unsigned long long)cp.payload_bytes);
   android_log_print(4,"RenderProfile","frames=%u draw_calls=%u tex_uploads=%u cache_hits=%u compressed_bytes=%u convert_us=%u upload_us=%u sub_updates=%u sub_update_us=%u",samples,draw_calls,texture_upload_count,texture_cache_hits,(unsigned)texture_bytes,(unsigned)texture_convert_us,(unsigned)texture_upload_us,texture_update_count,(unsigned)texture_update_us);
   android_log_print(4,"DrawProfile","frames=%u calls=%u submit_us=%llu",samples,draw_calls,(unsigned long long)draw_submit_us);
   graphics_profile_reset();android_log_print(4,"Timing","frame=%u mean_frame_us=%u mean_swap_us=%u keyboard=%d",frame,(unsigned)((after-window_start)/samples),(unsigned)(swap_total/samples),keyboard_active());window_start=after;swap_total=0;samples=0;}
 }
 if(observe_game)gameplay_experiment_present(frame);
 return 1;
}
static int initialized;
int graphics_initialize(void){
 if(initialized)return 0;
 vglSetDisplayCallback(launcher_display_overlay);
 /* Pinned vitaGL returns resolution fallback, not success/failure. */
 GLboolean fallback=vglInitWithCustomSizes(0,960,544,64*1024*1024,64*1024*1024,0,0,SCE_GXM_MULTISAMPLE_NONE);
 GLint viewport[4]={0};glGetIntegerv(GL_VIEWPORT,viewport);
 GLenum error=glGetError();
 android_log_print(4,"GPU","Resolution fallback=%u viewport=%d,%d %dx%d error=0x%x",
  (unsigned)fallback,viewport[0],viewport[1],viewport[2],viewport[3],(unsigned)error);
 if(error||viewport[2]!=960||viewport[3]!=544)return -1;
 /* A configured size is not proof that vitaGL created its allocator.
  * Probe each specific pool: vglAlloc has no external-heap fallback. */
 void *ram_probe=vglAlloc(256,VGL_MEM_RAM),*vram_probe=vglAlloc(256,VGL_MEM_VRAM);
 int pools_ready=ram_probe&&vram_probe;
 android_log_print(4,"GPU","Dedicated pools created: RAM=%u CDRAM=%u",ram_probe!=NULL,vram_probe!=NULL);
 vglFree(ram_probe);vglFree(vram_probe);
 if(!pools_ready)return -1;
 /* Stats are safe only after both allocator handles have been proved valid. */
 android_log_print(4,"GPU","Dedicated pools free: RAM=%u CDRAM=%u",(unsigned)vglMemFree(VGL_MEM_RAM),(unsigned)vglMemFree(VGL_MEM_VRAM));
 glHint(GL_TEXTURE_COMPRESSION_HINT,GL_NICEST);
 initialized=1;return 0;
}
static int window_geometry(void *window,int width,int height,int format){
 (void)format;
 if(!window||!initialized||width<0||height<0)return -1;
 return (width&&width!=960)||(height&&height!=544)?-1:0;
}
static void *proc(const char *name){return (void *)graphics_lookup(name);}
static uintptr_t graphics_resolve(const char *name){
 if(!name)return 0;
 if(!strcmp(name,"glEnable"))return (uintptr_t)stream_enable;
 if(!strcmp(name,"glDisable"))return (uintptr_t)stream_disable;
 if(!strcmp(name,"glDepthMask"))return (uintptr_t)stream_depth_mask;
 if(!strcmp(name,"glBlendFunc"))return (uintptr_t)stream_blend;
 if(!strcmp(name,"glActiveTexture"))return (uintptr_t)stream_active_texture;
 if(!strcmp(name,"glUniform1i"))return (uintptr_t)stream_uniform_i;
 if(!strcmp(name,"glEnableVertexAttribArray"))return (uintptr_t)gt_enable;
 if(!strcmp(name,"glDisableVertexAttribArray"))return (uintptr_t)gt_disable;
 uintptr_t profiled=graphics_cpu_lookup(name);if(profiled)return profiled;
 if(!strcmp(name,"eglSwapBuffers"))return (uintptr_t)bridge_swap;
 if(!strcmp(name,"glCompressedTexImage2D"))return (uintptr_t)bridge_compressed_texture;
 if(!strcmp(name,"glTexSubImage2D"))return (uintptr_t)bridge_texture_update;
 if(!strcmp(name,"glDrawElements"))return (uintptr_t)bridge_draw_elements;
 if(!strcmp(name,"glDrawArrays"))return (uintptr_t)bridge_draw_arrays;
 if(!strcmp(name,"glGetError"))return (uintptr_t)bridge_get_error;
 if(!strcmp(name,"ANativeWindow_setBuffersGeometry"))return (uintptr_t)window_geometry;
 if(!strcmp(name,"eglGetProcAddress"))return (uintptr_t)proc;
#include "graphics_float_lookup.inc"
 if(!strncmp(name,"gl",2)||!strncmp(name,"egl",3))return (uintptr_t)vglGetProcAddress(name);
 return 0;
}
void graphics_recording_configure(int enabled){graphics_guard_configure(enabled);}
uintptr_t graphics_lookup(const char *name){
 uintptr_t target=graphics_resolve(name);
 if(!target||!name)return target;
 int covered=!strcmp(name,"glDepthMask")||!strcmp(name,"glEnable")||!strcmp(name,"glDisable")||!strcmp(name,"glBlendFunc")||!strcmp(name,"glActiveTexture")||!strcmp(name,"glUniform1i")||!strcmp(name,"glEnableVertexAttribArray")||!strcmp(name,"glDisableVertexAttribArray")||!strcmp(name,"glDrawElements")||!strcmp(name,"glDrawArrays");
 if(graphics_cpu_trace_available()&&(!strcmp(name,"glUseProgram")||!strcmp(name,"glBindTexture")||!strcmp(name,"glUniformMatrix4fv")||!strcmp(name,"glVertexAttribPointer")))covered=1;
 return graphics_guard_wrap(name,target,covered);
}
static char *read_asset(const char *root,const char *name,size_t *size){
 AndroidAssetManager manager;if(android_asset_manager_init(&manager,root))return NULL;
 AndroidAsset *asset=android_asset_open(&manager,name,2);if(!asset)return NULL;
 int64_t n=android_asset_length(asset);
 if(n<0||n>8*1024*1024){android_asset_close(asset);return NULL;}
 char *p=malloc((size_t)n+1);if(!p){android_asset_close(asset);return NULL;}
 if(android_asset_read(asset,p,(size_t)n)!=(int)n){free(p);android_asset_close(asset);return NULL;}
 android_asset_close(asset);p[n]=0;*size=(size_t)n;return p;
}
static GLuint shader(GLenum type,const char *source){
 GLuint s=glCreateShader(type);glShaderSource(s,1,&source,NULL);glCompileShader(s);
 GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char message[1024];glGetShaderInfoLog(s,sizeof(message),NULL,message);android_log_print(6,"GPU","Shader: %s",message);glDeleteShader(s);return 0;}
 return s;
}
/* Compile every shipped GLES pair without changing the active program.
 * This checks compiler/linker support, not the visual correctness of effects. */
int graphics_shader_probe(const char *root){
 static const char *pairs[][3]={
#define SHADER_PAIR(label,vertex,fragment) {label,vertex,fragment},
#include "graphics_shader_pairs.inc"
#undef SHADER_PAIR
 };
 int failures=0;
 for(unsigned i=0;i<sizeof(pairs)/sizeof(pairs[0]);i++){
  size_t n;char *vs=read_asset(root,pairs[i][1],&n),*fs=read_asset(root,pairs[i][2],&n);
  GLuint v=0,f=0,p=0;GLint linked=0;
  if(vs&&fs){
   v=shader(GL_VERTEX_SHADER,vs);f=shader(GL_FRAGMENT_SHADER,fs);
   if(v&&f){
    p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);
    glLinkProgram(p);glGetProgramiv(p,GL_LINK_STATUS,&linked);
    if(!linked){char message[1024]={0};glGetProgramInfoLog(p,sizeof(message),NULL,message);
     android_log_print(6,"GPU","Shader pair %s link: %s",pairs[i][0],message);}
   }
  }
  GLenum error=glGetError();int ok=vs&&fs&&v&&f&&p&&linked&&!error;
  android_log_print(ok?4:6,"GPU","Shader pair %s: %s error=0x%x",pairs[i][0],ok?"OK":"FAILED",error);
  if(!ok)failures++;
  if(p)glDeleteProgram(p);
  if(v)glDeleteShader(v);
  if(f)glDeleteShader(f);
  free(vs);free(fs);
 }
 return failures;
}
static uint32_t u32(const unsigned char *p){uint32_t v;memcpy(&v,p,4);return v;}
int graphics_probe(const char *root){
 size_t vs_n,fs_n,tex_n;char *vs=read_asset(root,"shaders/spritebatch.vs",&vs_n);
 char *fs=read_asset(root,"shaders/spritebatch.fs",&fs_n);
 unsigned char *tex=(unsigned char *)read_asset(root,"images/ui/tick.png",&tex_n);
 if(!vs||!fs||!tex){free(vs);free(fs);free(tex);return 1;}
 GLuint vert=shader(GL_VERTEX_SHADER,vs),frag=shader(GL_FRAGMENT_SHADER,fs);free(vs);free(fs);
 if(!vert||!frag){if(vert)glDeleteShader(vert);if(frag)glDeleteShader(frag);free(tex);return 2;}
 GLuint program=glCreateProgram();glAttachShader(program,vert);glAttachShader(program,frag);
 glBindAttribLocation(program,0,"a_position");glBindAttribLocation(program,1,"a_color");glBindAttribLocation(program,2,"a_texCoord0");
 glLinkProgram(program);GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);
 glDeleteShader(vert);glDeleteShader(frag);
 if(!linked){char msg[1024];glGetProgramInfoLog(program,sizeof(msg),NULL,msg);android_log_print(6,"GPU","Link: %s",msg);glDeleteProgram(program);free(tex);return 3;}
 if(tex_n<68||memcmp(tex,"\xabKTX 11\xbb\r\n\x1a\n",12)||u32(tex+12)!=0x04030201||u32(tex+28)!=0x9278||u32(tex+36)!=64||u32(tex+40)!=64){glDeleteProgram(program);free(tex);return 4;}
 uint32_t offset=64+u32(tex+60);
 if(offset<64||offset>tex_n-4||u32(tex+offset)!=4096||tex_n-offset-4<4096){glDeleteProgram(program);free(tex);return 5;}
 GLuint texture;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 bridge_compressed_texture(GL_TEXTURE_2D,0,GL_COMPRESSED_RGBA8_ETC2_EAC,64,64,0,4096,tex+offset+4);free(tex);
 GLenum err=glGetError();android_log_print(4,"GPU","Texture upload error: 0x%x",err);
 if(err){glDeleteTextures(1,&texture);glDeleteProgram(program);return 6;}
 const GLfloat identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 /* Position, colour, UV. Full-screen quad also tests the engine's colour-alpha adjustment. */
 const GLfloat vertices[]={-1,-1,0,1,1,1,1,1,0,1, 1,-1,0,1,1,1,1,1,1,1, -1,1,0,1,1,1,1,1,0,0, 1,1,0,1,1,1,1,1,1,0};
 GLuint vbo;glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
 glUseProgram(program);glUniformMatrix4fv(glGetUniformLocation(program,"u_projTrans"),1,GL_FALSE,identity);glUniform1i(glGetUniformLocation(program,"u_texture"),0);
 glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glEnableVertexAttribArray(2);
 glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,10*sizeof(GLfloat),(void *)0);
 glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,10*sizeof(GLfloat),(void *)(4*sizeof(GLfloat)));
 glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,10*sizeof(GLfloat),(void *)(8*sizeof(GLfloat)));
 glViewport(0,0,960,544);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
 glDrawArrays(GL_TRIANGLE_STRIP,0,4);glFinish();
 unsigned char *pixels=malloc(960*544*4);unsigned lit=0;int has_pixels=pixels!=NULL;
 if(pixels){glReadPixels(0,0,960,544,GL_RGBA,GL_UNSIGNED_BYTE,pixels);for(unsigned i=0;i<960*544;i++)if(pixels[i*4]||pixels[i*4+1]||pixels[i*4+2])lit++;free(pixels);}
 err=glGetError();android_log_print(4,"GPU","Readback: %u nonblack pixels, error 0x%x",lit,err);
 glClearColor(16.0f/255,20.0f/255,25.0f/255,1);glClear(GL_COLOR_BUFFER_BIT);
 vglSwapBuffers(keyboard_active()?GL_TRUE:GL_FALSE);
 glDisableVertexAttribArray(0);glDisableVertexAttribArray(1);glDisableVertexAttribArray(2);
 glBindBuffer(GL_ARRAY_BUFFER,0);glUseProgram(0);glDeleteBuffers(1,&vbo);glDeleteProgram(program);glDeleteTextures(1,&texture);
 return !has_pixels?7:err?8:lit<100?9:0;
}

/* Establish the state previously left by the diagnostic quad, without compiling
 * throwaway shaders or allocating/readback of a test texture. */
int graphics_prepare_game(void){
 glViewport(0,0,960,544);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
 glDisableVertexAttribArray(0);glDisableVertexAttribArray(1);glDisableVertexAttribArray(2);
 glBindBuffer(GL_ARRAY_BUFFER,0);glUseProgram(0);
 glClearColor(16.0f/255,20.0f/255,25.0f/255,1);glClear(GL_COLOR_BUFFER_BIT);
 vglSwapBuffers(GL_FALSE);
 return glGetError()!=GL_NO_ERROR;
}
