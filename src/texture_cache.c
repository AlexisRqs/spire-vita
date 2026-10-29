#include "texture_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifdef __vita__
#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#else
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif
#ifndef CACHE_CLOCK
static uint64_t cache_clock(void){
#ifdef __vita__
 return sceKernelGetProcessTimeWide();
#else
 struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000+t.tv_nsec/1000;
#endif
}
#define CACHE_CLOCK cache_clock
#endif
static TextureCacheProfile cache_profile;
uint64_t texture_cache_hash(const void *data,size_t n){
 const unsigned char *p=data;uint64_t h=UINT64_C(14695981039346656037);
 while(n--){h^=*p++;h*=UINT64_C(1099511628211);}return h;
}
/* The pack owns a separate descriptor: never mix stdio buffering with these
 * seeks. The caller holds cache_lock for the complete seek/read transaction. */
static int pack_open_fd(const char *path){
#ifdef __vita__
 return sceIoOpen(path,SCE_O_RDONLY,0);
#else
 return open(path,O_RDONLY);
#endif
}
static void pack_close_fd(int fd){
#ifdef __vita__
 sceIoClose(fd);
#else
 close(fd);
#endif
}
static int64_t pack_seek_fd(int fd,int offset,int whence){
#ifdef __vita__
 return sceIoLseek32(fd,offset,whence);
#else
 return lseek(fd,offset,whence);
#endif
}
static size_t pack_read_exact(int fd,void *out,size_t n){
 size_t done=0;
 while(done<n){
#ifdef __vita__
  int got=sceIoRead(fd,(char*)out+done,n-done);
#else
  ssize_t got=read(fd,(char*)out+done,n-done);
  if(got<0&&errno==EINTR)continue;
#endif
  if(got<=0)break;
  done+=(size_t)got;
 }
 return done;
}
typedef struct {uint64_t key;uint32_t e[6];} CacheEntry;
_Static_assert(sizeof(CacheEntry)==32,"cache disk entry layout");
static struct {char root[256];CacheEntry *entries;unsigned count;int data;} pack={.data=-1};
static pthread_mutex_t cache_lock=PTHREAD_MUTEX_INITIALIZER;
static void reset_locked(void){if(pack.data>=0)pack_close_fd(pack.data);free(pack.entries);memset(&pack,0,sizeof(pack));pack.data=-1;}
void texture_cache_reset(void){pthread_mutex_lock(&cache_lock);reset_locked();pthread_mutex_unlock(&cache_lock);}
static int compare(uint64_t key,unsigned fmt,unsigned w,unsigned h,const CacheEntry *v){
 return key<v->key?-1:key>v->key?1:fmt<v->e[0]?-1:fmt>v->e[0]?1:w<v->e[1]?-1:w>v->e[1]?1:h<v->e[2]?-1:h>v->e[2]?1:0;
}
static int packed_open(const char *root,uint64_t hash,unsigned fmt,unsigned w,unsigned h,uint32_t *length){
 char path[256];
 if(strcmp(pack.root,root))reset_locked();
 if(!pack.entries){
  if(strlen(root)>=sizeof(pack.root)||snprintf(path,sizeof(path),"%s/index.bin",root)>=(int)sizeof(path))return -1;
  FILE *idx=fopen(path,"rb");if(!idx)return -1;
  uint32_t header[4];CacheEntry *entries=NULL;int data=-1;
  if(fread(header,1,16,idx)!=16||header[0]!=0x33334954||!header[1]||header[1]>4096||header[2]>0x40000000)goto failed;
  entries=malloc(header[1]*sizeof(*entries));if(!entries)goto failed;
  if(fread(entries,sizeof(*entries),header[1],idx)!=header[1]||fgetc(idx)!=EOF||ferror(idx))goto failed;
  for(unsigned i=0;i<header[1];i++){
   uint32_t *e=entries[i].e;
   if(e[4]<40||e[3]>header[2]||e[4]>header[2]-e[3])goto failed;
   if(i&&compare(entries[i-1].key,entries[i-1].e[0],entries[i-1].e[1],entries[i-1].e[2],entries+i)>=0)goto failed;
  }
  if(snprintf(path,sizeof(path),"%s/data.bin",root)>=(int)sizeof(path))goto failed;
  data=pack_open_fd(path);if(data<0)goto failed;
  if(pack_seek_fd(data,0,SEEK_END)!=(int64_t)header[2])goto failed;
  fclose(idx);pack.entries=entries;pack.count=header[1];pack.data=data;strcpy(pack.root,root);goto ready;
 failed: fclose(idx);if(data>=0)pack_close_fd(data);free(entries);return -1;
 }
 ready:;
 unsigned lo=0,hi=pack.count;
 while(lo<hi){unsigned mid=lo+(hi-lo)/2;CacheEntry *entry=pack.entries+mid;
  int cmp=compare(hash,fmt,w,h,entry);
  if(cmp>0)lo=mid+1;else if(cmp<0)hi=mid;else{
   if(pack_seek_fd(pack.data,entry->e[3],SEEK_SET)!=(int64_t)entry->e[3])return -1;
   *length=entry->e[4];return pack.data;
  }
 }
 return -1;
}
static void *load_locked(const char *root,unsigned format,unsigned width,unsigned height,const void *source,size_t size,unsigned *ow,unsigned *oh,size_t *os){
 if(!root||!source||!ow||!oh||!os||!width||!height||width>4096||height>4096||(format!=0x9274&&format!=0x9276&&format!=0x9278))return NULL;
 if(size!=(size_t)((width+3)/4)*((height+3)/4)*(format==0x9278?16:8))return NULL;
 uint64_t start=CACHE_CLOCK();
 uint64_t hash=texture_cache_hash(source,size);cache_profile.source_hash_us+=CACHE_CLOCK()-start;cache_profile.source_bytes+=size;
 char path[256];
 int n=snprintf(path,sizeof(path),"%s/%x-%ux%u-%016llx.bin",root,format,width,height,(unsigned long long)hash);
 if(n<0||(size_t)n>=sizeof(path))return NULL;
 start=CACHE_CLOCK();
 uint32_t packed_size=0;int fd=packed_open(root,hash,format,width,height,&packed_size);
 FILE *f=fd<0?fopen(path,"rb"):NULL;
 cache_profile.lookup_us+=CACHE_CLOCK()-start;
 if(fd<0&&!f)return NULL;
 uint32_t h[8];uint64_t checksum;void *out=NULL,*packed_buffer=NULL;
 unsigned dw=width,dh=height;
 if(width>512||height>512){dw=(width+1)/2;dh=(height+1)/2;}
 unsigned expected=(dw+3)/4*((dh+3)/4)*(format==0x9274?8:16);
 if(fd>=0){
  if(packed_size!=40+expected)goto done;
  packed_buffer=malloc(packed_size);if(!packed_buffer)goto done;
 }
 start=CACHE_CLOCK();
 int header_ok;
 if(fd>=0){
  size_t got=pack_read_exact(fd,packed_buffer,packed_size);
  cache_profile.payload_bytes+=got>40?got-40:0;
  header_ok=got==packed_size;
  if(header_ok){memcpy(h,packed_buffer,32);memcpy(&checksum,(char*)packed_buffer+32,8);}
 }else header_ok=fread(h,1,sizeof(h),f)==sizeof(h)&&fread(&checksum,1,8,f)==8;
 cache_profile.read_us+=CACHE_CLOCK()-start;
 if(!header_ok)goto done;
 if(h[0]!=0x33334354||h[1]!=format||h[2]!=width||h[3]!=height)goto done;
 if(h[4]!=dw||h[5]!=dh||h[7]!=(uint32_t)size||h[6]!=expected)goto done;
 if(fd>=0){
  /* Return the allocation base so the caller can free it normally. Only 40
   * extra bytes are retained, and one contiguous read covers the entire entry. */
  out=packed_buffer;packed_buffer=NULL;memmove(out,(char*)out+40,h[6]);
 }else{
  out=malloc(h[6]);if(!out)goto done;
  start=CACHE_CLOCK();size_t got=fread(out,1,h[6],f);
  int read_ok=got==h[6]&&fgetc(f)==EOF&&!ferror(f);
  cache_profile.read_us+=CACHE_CLOCK()-start;cache_profile.payload_bytes+=got;
  if(!read_ok){free(out);out=NULL;goto done;}
 }
 start=CACHE_CLOCK();uint64_t actual=texture_cache_hash(out,h[6]);
 cache_profile.checksum_us+=CACHE_CLOCK()-start;
 if(actual!=checksum){free(out);out=NULL;goto done;}
 *ow=dw;*oh=dh;*os=h[6];
 done:free(packed_buffer);if(f)fclose(f);return out;
}

void *texture_cache_load(const char *root,unsigned format,unsigned width,unsigned height,const void *source,size_t size,unsigned *ow,unsigned *oh,size_t *os){
 uint64_t start=CACHE_CLOCK();pthread_mutex_lock(&cache_lock);
 uint64_t locked=CACHE_CLOCK();cache_profile.wait_us+=locked-start;cache_profile.attempts++;
 void *result=load_locked(root,format,width,height,source,size,ow,oh,os);
 cache_profile.total_us+=CACHE_CLOCK()-locked;cache_profile.hits+=result!=NULL;
 pthread_mutex_unlock(&cache_lock);return result;
}

TextureCacheProfile texture_cache_profile_take(void){
 pthread_mutex_lock(&cache_lock);TextureCacheProfile out=cache_profile;
 memset(&cache_profile,0,sizeof(cache_profile));pthread_mutex_unlock(&cache_lock);return out;
}
