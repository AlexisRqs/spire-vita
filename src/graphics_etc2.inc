/* Bounded ETC2 conversion avoids vitaGL full RGBA + POT padding buffers.
 * Uses linked detex/stb; native DXT upload owns swizzle and GPU storage. */
#include "etc2_transcode.h"
#include "texture_cache.h"
static unsigned texture_cache_hits;
static uint64_t texture_convert_us,texture_upload_us,texture_bytes;
static unsigned texture_upload_count;
static GLenum bridge_error;
static void bridge_set_error(GLenum error){if(!bridge_error)bridge_error=error;}
static GLenum bridge_get_error(void){if(bridge_error){GLenum error=bridge_error;bridge_error=0;return error;}return glGetError();}
static void bridge_compressed_texture(GLenum target,GLint level,GLenum format,GLsizei width,GLsizei height,GLint border,GLsizei size,const void *data){
 /* Direct-cache DXT keeps logical dimensions in the managed texture.
  * Only this validated payload path selects the smaller GPU allocation. */
 if(format==0x83f0||format==0x83f3){
  if(target!=GL_TEXTURE_2D){bridge_set_error(GL_INVALID_ENUM);return;}
  if(level!=0||width<=0||height<=0||width>4096||height>4096||border||!data||size<0){bridge_set_error(GL_INVALID_VALUE);return;}
  unsigned dw=width,dh=height;if(width>512||height>512){dw=(width+1)/2;dh=(height+1)/2;}
  size_t expected=(size_t)((dw+3)/4)*((dh+3)/4)*(format==0x83f0?8:16);
  if((size_t)size!=expected){bridge_set_error(GL_INVALID_VALUE);return;}
  uint64_t begin=sceKernelGetProcessTimeWide();
  glCompressedTexImage2D(target,level,format,dw,dh,0,size,data);
  texture_upload_us+=sceKernelGetProcessTimeWide()-begin;texture_bytes+=size;texture_upload_count++;return;
 }
 if(format!=0x9274&&format!=0x9276&&format!=0x9278){glCompressedTexImage2D(target,level,format,width,height,border,size,data);return;}
 if(target!=GL_TEXTURE_2D){bridge_set_error(GL_INVALID_ENUM);return;}
 if(level<0||width<=0||height<=0||width>4096||height>4096||border||size<0||
    (size_t)size!=(size_t)((width+3)/4)*((height+3)/4)*(format==0x9278?16:8)){bridge_set_error(GL_INVALID_VALUE);return;}
 GLint storage=format==0x9274?GL_COMPRESSED_RGB_S3TC_DXT1_EXT:GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
 size_t output_size=(size_t)((width+3)/4)*((height+3)/4)*(format==0x9274?8:16);
 if(!data){glCompressedTexImage2D(target,level,storage,width,height,0,(GLsizei)output_size,NULL);return;}
 /* Game keeps original logical atlas dimensions; only physical GPU storage
  * changes. All shipped KTX assets have one mip level. */
 unsigned physical_w=width,physical_h=height;size_t cached_size=0;
 uint64_t cache_start=sceKernelGetProcessTimeWide();
 void *cached=level==0?texture_cache_load("ux0:data/spireandroid/texture-cache-033",format,width,height,data,size,&physical_w,&physical_h,&cached_size):NULL;
 if(cached){
  uint64_t begin_upload=sceKernelGetProcessTimeWide();
  glCompressedTexImage2D(target,level,storage,physical_w,physical_h,0,cached_size,cached);
  uint64_t end=sceKernelGetProcessTimeWide();free(cached);
  texture_convert_us+=begin_upload-cache_start;texture_upload_us+=end-begin_upload;texture_bytes+=cached_size;texture_upload_count++;texture_cache_hits++;
  if(end-cache_start>250000)android_log_print(4,"CacheTiming","%dx%d physical=%ux%u read_us=%u upload_us=%u",width,height,physical_w,physical_h,(unsigned)(begin_upload-cache_start),(unsigned)(end-begin_upload));
  return;
 }
 void *pixels=malloc(output_size);
 if(!pixels){bridge_set_error(GL_OUT_OF_MEMORY);return;}
 uint64_t conversion_start=sceKernelGetProcessTimeWide();
 if(etc2_transcode(format,width,height,data,size,pixels,output_size)){free(pixels);bridge_set_error(GL_INVALID_VALUE);return;}
 uint64_t upload_start=sceKernelGetProcessTimeWide();
 glCompressedTexImage2D(target,level,storage,width,height,0,(GLsizei)output_size,pixels);
 uint64_t end=sceKernelGetProcessTimeWide();
 texture_convert_us+=upload_start-conversion_start;texture_upload_us+=end-upload_start;
 texture_bytes+=output_size;texture_upload_count++;
 if(end-conversion_start>250000)android_log_print(4,"TextureTiming","%dx%d fmt=0x%x level=%d convert_us=%u upload_us=%u",width,height,format,level,(unsigned)(upload_start-conversion_start),(unsigned)(end-upload_start));
 free(pixels);
}
