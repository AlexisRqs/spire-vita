#include "android_assets.h"
#include "scene_metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
static AndroidAssetOverride asset_override;
void android_asset_bind_override(AndroidAssetOverride loader){asset_override=loader;}
struct AndroidAsset {FILE *file;int64_t length;unsigned char *memory;size_t position;};
int android_asset_manager_init(AndroidAssetManager *manager,const char *root){
 if(!manager||!root||!*root||strlen(root)>=sizeof(manager->root))return -1;
 strcpy(manager->root,root);size_t n=strlen(root);
 while(n>1&&manager->root[n-1]=='/')manager->root[--n]=0;
 return 0;
}
static int relative_name(const char *name){
 if(!name||!*name||*name=='/'||strchr(name,':')||strchr(name,'\\'))return 0;
 const char *p=name;
 while(*p){const char *end=strchr(p,'/');size_t n=end?(size_t)(end-p):strlen(p);
  if(!n||(n==1&&p[0]=='.')||(n==2&&p[0]=='.'&&p[1]=='.'))return 0;
  if(!end)return 1;
  p=end+1;
 }
 return 0;
}
AndroidAsset *android_asset_open(AndroidAssetManager *manager,const char *name,int mode){
 if(!manager||!manager->root[0]||!relative_name(name)||mode<0||mode>3)return NULL;
 char path[2048];int n=snprintf(path,sizeof(path),"%s/%s",manager->root,name);
 if(n<0||(size_t)n>=sizeof(path))return NULL;
 /* Optional private subset; original remains the fallback. */
 FILE *file=NULL;const char *effective=name;char effective_sony[256];
 /* Android selects Xbox/DirectInput glyphs for the Vita adapter. The bundled
  * PS4 assets use the same semantic filenames (a=cross, b=circle, ...). */
 const char *glyph=NULL;
 if(!strncmp(name,"images/ui/controller/xbox360/",29))glyph=name+29;
 else if(!strncmp(name,"images/ui/controller/xboxOne/",29))glyph=name+29;
 if(glyph&&*glyph&&!strchr(glyph,'/')){
  char sony[2048];int count=snprintf(sony,sizeof(sony),"%s/images/ui/controller/ps4/%s",manager->root,glyph);
  if(count>=0&&(size_t)count<sizeof(sony))file=fopen(sony,"rb");
  if(file){snprintf(effective_sony,sizeof(effective_sony),"images/ui/controller/ps4/%s",glyph);effective=effective_sony;}
 }
 if(!strcmp(name,"font/zhs/NotoSansMonoCJKsc-Regular.otf")){
  char reduced[2048];int count=snprintf(reduced,sizeof(reduced),"%s/font/zhs/NotoSansMonoCJKsc-Regular.vita.otf",manager->root);
  if(count>=0&&(size_t)count<sizeof(reduced))file=fopen(reduced,"rb");
 }
 if(!file)file=fopen(path,"rb");
 if(!file)return NULL;
 struct stat st;
 if(fstat(fileno(file),&st)||!S_ISREG(st.st_mode)||st.st_size<0){fclose(file);return NULL;}
 /* This SDK's unbuffered fread refills directly into the caller's buffer.
  * Avoid repeated small-buffer copies for whole-file asset reads. Set before
  * any stream IO; failure leaves the ordinary buffered path available. */
 (void)setvbuf(file,NULL,_IONBF,0);
 AndroidAsset *a=malloc(sizeof(*a));if(!a){fclose(file);return NULL;}
 a->file=file;a->length=(int64_t)st.st_size;a->memory=NULL;a->position=0;
 if(asset_override){size_t bytes=0;a->memory=asset_override(effective,file,&bytes);
  if(a->memory){a->length=bytes;fclose(file);a->file=NULL;}
  else{clearerr(file);if(fseek(file,0,SEEK_SET)){fclose(file);free(a);return NULL;}}
 }
 return a;
}
int64_t android_asset_length(AndroidAsset *asset){return asset?asset->length:0;}
int android_asset_read(AndroidAsset *asset,void *buffer,size_t count){
 if(!asset||(!buffer&&count))return -1;
 if(!count)return 0;
 if(count>INT_MAX)count=INT_MAX;
 if(asset->memory){size_t left=(size_t)asset->length-asset->position;if(count>left)count=left;
  if(count)memcpy(buffer,asset->memory+asset->position,count);
  asset->position+=count;return (int)count;}
 uint64_t start=scene_clock();size_t n=fread(buffer,1,count,asset->file);
 scene_asset_io(scene_clock()-start,(unsigned)n);
 return n?(int)n:ferror(asset->file)?-1:0;
}
void android_asset_close(AndroidAsset *asset){if(asset){if(asset->file)fclose(asset->file);free(asset->memory);free(asset);}}
uintptr_t android_assets_lookup(const char *name){
 if(!strcmp(name,"AAssetManager_open"))return (uintptr_t)android_asset_open;
 if(!strcmp(name,"AAsset_getLength64"))return (uintptr_t)android_asset_length;
 if(!strcmp(name,"AAsset_read"))return (uintptr_t)android_asset_read;
 if(!strcmp(name,"AAsset_close"))return (uintptr_t)android_asset_close;
 return 0;
}
