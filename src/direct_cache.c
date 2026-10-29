#include "direct_cache.h"
#include <direct_cache_identity.h>
#include "android_log.h"
#include "scene_metrics.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
/* Disk layout is explicit little endian, never a dumped native structure. */
#define RECORD 304u
#define MAX_SOURCE (32u*1024*1024)
static unsigned char *index_data;
static unsigned entry_count;
/* Native bytes only: no managed object or profile state is retained. */
static unsigned char *loading_data;
static const unsigned char **loading_entries;
static const char *loading_path;
static unsigned loading_reads,loading_attempted;
static void loading_release(void){free(loading_entries);free(loading_data);loading_entries=NULL;loading_data=NULL;}

static FILE *pack_file,*receipt_file;
typedef struct {uint64_t size,mtime,ctime;} Stamp;
static Stamp *stamps;
static unsigned char *verified;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static struct {unsigned hits,misses,validated,invalid;uint64_t avoided,read_us,validate_us;} metrics;
static uint32_t u32(const unsigned char *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t u64(const unsigned char *p){return u32(p)|(uint64_t)u32(p+4)<<32;}
static void put32(unsigned char *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=n>>(8*i);}
static void put64(unsigned char *p,uint64_t n){put32(p,n);put32(p+4,n>>32);}
static uint64_t hash_more(uint64_t h,const void *data,size_t n){const unsigned char *p=data;while(n--){h^=*p++;h*=UINT64_C(1099511628211);}return h;}
static uint64_t hash(const void *p,size_t n){return hash_more(UINT64_C(14695981039346656037),p,n);}
static int safe_name(const char *p){
 if(!*p||*p=='/'||strchr(p,':')||strchr(p,'\\'))return 0;
 while(*p){const char *q=strchr(p,'/');size_t n=q?(size_t)(q-p):strlen(p);
  if(!n||(n==1&&*p=='.')||(n==2&&p[0]=='.'&&p[1]=='.'))return 0;
  if(!q)return 1;
  p=q+1;if(!*p)return 0;
 }return 0;
}
static int stamp(FILE *f,Stamp *s){struct stat st;if(fstat(fileno(f),&st)||!S_ISREG(st.st_mode)||st.st_size<0)return -1;
 *s=(Stamp){(uint64_t)st.st_size,(uint64_t)st.st_mtime,(uint64_t)st.st_ctime};return 0;}
static int same(Stamp a,Stamp b){return a.size==b.size&&a.mtime==b.mtime&&a.ctime==b.ctime;}
void direct_cache_shutdown(void){
 loading_release();
 if(pack_file)fclose(pack_file);
 if(receipt_file)fclose(receipt_file);
 pack_file=receipt_file=NULL;free(index_data);free(stamps);free(verified);index_data=NULL;stamps=NULL;verified=NULL;entry_count=0;
}
int direct_cache_init(const char *idx,const char *pack,const char *receipt){
 direct_cache_shutdown();loading_attempted=0;memset(&metrics,0,sizeof(metrics));
 FILE *f=fopen(idx,"rb");if(!f)return -1;
 unsigned char h[24];int result=-1;
 if(fread(h,1,24,f)!=24||u32(h)!=0x31435053||u32(h+4)!=1||u32(h+12)!=RECORD||u64(h+16)!=DIRECT_CACHE_ID)goto end;
 entry_count=u32(h+8);if(!entry_count||entry_count>4096)goto end;
 index_data=malloc(entry_count*RECORD);stamps=calloc(entry_count,sizeof(*stamps));verified=calloc(entry_count,1);
 if(!index_data||!stamps||!verified)goto end;
 if(fread(index_data,RECORD,entry_count,f)!=entry_count||fgetc(f)!=EOF||ferror(f)||hash(index_data,entry_count*RECORD)!=DIRECT_CACHE_ID)goto end;
 pack_file=fopen(pack,"rb");if(!pack_file)goto end;
 (void)setvbuf(pack_file,NULL,_IONBF,0);Stamp ps;if(stamp(pack_file,&ps)||ps.size>INT_MAX)goto end;
 for(unsigned i=0;i<entry_count;i++){
  unsigned char *r=index_data+i*RECORD;unsigned fmt=u32(r+284),w=u32(r+288),height=u32(r+292),dw=u32(r+296),dh=u32(r+300);
  if(!memchr(r,0,256)||!safe_name((char*)r)||(i&&strcmp((char*)r-RECORD,(char*)r)>=0)||!w||!height||w>4096||height>4096||
     (fmt!=0x9274&&fmt!=0x9276&&fmt!=0x9278)||u32(r+272)<68||u32(r+272)>MAX_SOURCE||
     dw!=(w>512||height>512?(w+1)/2:w)||dh!=(w>512||height>512?(height+1)/2:height)||
     u32(r+280)!=40+(dw+3)/4*((dh+3)/4)*(fmt==0x9274?8:16)||
     (uint64_t)u32(r+276)+u32(r+280)>ps.size)goto end;
 }
 /* Receipts are disposable accelerators, keyed by the compiled dataset.
  * They are NOT authentication against edits preserving size and timestamps. */
 receipt_file=fopen(receipt,"rb");
 unsigned char rh[16];put64(rh,DIRECT_CACHE_ID);put64(rh+8,hash(rh,8));
 int valid_header=0;
 if(receipt_file){unsigned char disk[16];valid_header=fread(disk,1,16,receipt_file)==16&&!memcmp(disk,rh,16);
  if(valid_header){unsigned char b[40];unsigned n=0;while(n++<entry_count*4&&fread(b,1,40,receipt_file)==40){unsigned i=u32(b);
   if(i<entry_count&&!u32(b+4)&&u64(b+32)==hash(b,32)){stamps[i]=(Stamp){u64(b+8),u64(b+16),u64(b+24)};verified[i]=1;}
  }}fclose(receipt_file);receipt_file=NULL;
 }
 /* Rewrite a compact snapshot, including only complete checked records. */
 char temp[512];int len=snprintf(temp,sizeof(temp),"%s.tmp",receipt);
 if(len>0&&(size_t)len<sizeof(temp)){
  FILE *out=fopen(temp,"wb");if(out){int ok=fwrite(rh,1,16,out)==16;
   for(unsigned i=0;ok&&i<entry_count;i++)if(verified[i]){unsigned char b[40]={0};put32(b,i);put64(b+8,stamps[i].size);put64(b+16,stamps[i].mtime);put64(b+24,stamps[i].ctime);put64(b+32,hash(b,32));ok=fwrite(b,1,40,out)==40;}
   if(fclose(out))ok=0;
   if(ok&&!rename(temp,receipt))receipt_file=fopen(receipt,"ab");else remove(temp);
  }
 }
 result=0;
end:fclose(f);if(result)direct_cache_shutdown();return result;
}
/* Called around the original initial-profile operation, including preload cost.
 * A malformed bundle is rejected in full before publishing any pointer. */
void direct_cache_loading_path(const char *path){loading_path=path;}
void direct_cache_loading_begin(void){
 pthread_mutex_lock(&mutex);
 loading_release();loading_reads=0;
 FILE *f=NULL;unsigned char *data=NULL;const unsigned char **entries=NULL;
 uint64_t start=scene_clock();
 if(!loading_path||!index_data||loading_attempted)goto done;
 /* CloudInit may be called again after profile reload with no library work.
  * Only the first eligible call per cache lifetime may prefetch. */
 loading_attempted=1;
 f=fopen(loading_path,"rb");if(!f)goto done;
 Stamp st;if(stamp(f,&st)||st.size<24||st.size>20u*1024*1024)goto done;
 data=malloc((size_t)st.size);entries=calloc(entry_count,sizeof(*entries));
 if(!data||!entries)goto done;
 (void)setvbuf(f,NULL,_IONBF,0);
 if(fread(data,1,(size_t)st.size,f)!=st.size||fgetc(f)!=EOF||ferror(f))goto done;
 if(u32(data)!=0x314c5053||u32(data+4)!=1||u64(data+8)!=DIRECT_CACHE_ID||u32(data+20)!=st.size||u32(data+16)>entry_count)goto done;
 size_t at=24;
 for(unsigned j=0;j<u32(data+16);j++){
  if(st.size-at<8)goto done;
  unsigned i=u32(data+at),n=u32(data+at+4);at+=8;
  if(i>=entry_count||entries[i]||n>st.size-at)goto done;
  const unsigned char *r=index_data+i*RECORD;
  if((strncmp((const char*)r,"images/relics/",14)&&strncmp((const char*)r,"images/characters/",18))||u32(r+284)==0x9276||n!=u32(r+280)||hash(data+at,n)!=u64(r+264))goto done;
  entries[i]=data+at;at+=n;
 }
 if(at!=st.size)goto done;
 loading_data=data;loading_entries=entries;data=NULL;entries=NULL;
done:
 if(f)fclose(f);
 free(data);free(entries);
 android_log_print(4,"LoadingPack","active=%u prepare_us=%llu",loading_data!=NULL,(unsigned long long)(scene_clock()-start));
 pthread_mutex_unlock(&mutex);
}
void direct_cache_loading_end(void){
 pthread_mutex_lock(&mutex);
 android_log_print(4,"LoadingPack","pack_reads_avoided=%u",loading_reads);
 loading_release();pthread_mutex_unlock(&mutex);
}
static int validate_source(unsigned i,FILE *source){
 Stamp before,after;if(stamp(source,&before))return -1;
 const unsigned char *r=index_data+i*RECORD;if(before.size!=u32(r+272))return -1;
 if(verified[i]&&same(before,stamps[i]))return 0;
 uint64_t start=scene_clock(),digest=UINT64_C(14695981039346656037);unsigned char buffer[16384];size_t total=0,n;
 if(fseek(source,0,SEEK_SET))return -1;
 while((n=fread(buffer,1,sizeof(buffer),source))>0){digest=hash_more(digest,buffer,n);total+=n;if(total>MAX_SOURCE)break;}
 int ok=!ferror(source)&&total==before.size&&digest==u64(r+256)&&!stamp(source,&after)&&same(before,after);
 clearerr(source);if(fseek(source,0,SEEK_SET))ok=0;
 metrics.validate_us+=scene_clock()-start;
 if(!ok){verified[i]=0;return -1;}
 stamps[i]=before;verified[i]=1;metrics.validated++;
 if(receipt_file){unsigned char b[40]={0};put32(b,i);put64(b+8,before.size);put64(b+16,before.mtime);put64(b+24,before.ctime);put64(b+32,hash(b,32));
  if(fwrite(b,1,40,receipt_file)!=40||fflush(receipt_file)){fclose(receipt_file);receipt_file=NULL;}
 }
 return 0;
}
void *direct_cache_read(const char *name,FILE *source,size_t *size){
 if(!name||!source||!size)return NULL;
 pthread_mutex_lock(&mutex);void *out=NULL;unsigned char *entry=NULL;
 if(!index_data)goto end;
 unsigned lo=0,hi=entry_count;
 while(lo<hi){unsigned mid=lo+(hi-lo)/2;int c=strcmp(name,(char*)index_data+mid*RECORD);if(c>0)lo=mid+1;else hi=mid;}
 if(lo==entry_count||strcmp(name,(char*)index_data+lo*RECORD))goto end;
 const unsigned char *r=index_data+lo*RECORD;
 /* Keep the original punch-through-alpha enum/format contract. This first
  * direct path only maps RGB and full RGBA; 17 alpha1 assets use old cache. */
 if(u32(r+284)==0x9276)goto end;
 if(validate_source(lo,source)){metrics.invalid++;goto end;}
 unsigned bytes=u32(r+280);entry=malloc(bytes);if(!entry)goto end;
 uint64_t start=scene_clock();int ok;
 if(loading_entries&&loading_entries[lo]){memcpy(entry,loading_entries[lo],bytes);loading_reads++;ok=1;}
 else ok=!fseek(pack_file,u32(r+276),SEEK_SET)&&fread(entry,1,bytes,pack_file)==bytes;
 metrics.read_us+=scene_clock()-start;
 if(!ok||hash(entry,bytes)!=u64(r+264)){clearerr(pack_file);metrics.invalid++;goto end;}
 unsigned fmt=u32(r+284),w=u32(r+288),h=u32(r+292),payload=bytes-40;
 if(u32(entry)!=0x33334354||u32(entry+4)!=fmt||u32(entry+8)!=w||u32(entry+12)!=h||u32(entry+16)!=u32(r+296)||u32(entry+20)!=u32(r+300)||u32(entry+24)!=payload)goto end;
 /* A real DXT KTX. Engine keeps logical dimensions; bridge validates physical
  * payload size and applies the same half-size storage rule as cache 0.33. */
 out=malloc(68+payload);if(!out)goto end;
 unsigned char *b=out;memset(b,0,68);memcpy(b,"\253KTX 11\273\r\n\032\n",12);
 put32(b+12,0x04030201);put32(b+20,1);put32(b+28,fmt==0x9274?0x83f0:0x83f3);put32(b+32,fmt==0x9274?0x1907:0x1908);
 put32(b+36,w);put32(b+40,h);put32(b+52,1);put32(b+56,1);put32(b+64,payload);memcpy(b+68,entry+40,payload);
 *size=68+payload;metrics.hits++;metrics.avoided+=u32(r+272);
end:free(entry);if(!out)metrics.misses++;pthread_mutex_unlock(&mutex);return out;
}
void direct_cache_report(void){pthread_mutex_lock(&mutex);
 android_log_print(4,"DirectCache","hits=%u misses=%u validated=%u invalid=%u source_bytes_bypassed=%llu pack_read_us=%llu validation_us=%llu",metrics.hits,metrics.misses,metrics.validated,metrics.invalid,(unsigned long long)metrics.avoided,(unsigned long long)metrics.read_us,(unsigned long long)metrics.validate_us);
 memset(&metrics,0,sizeof(metrics));pthread_mutex_unlock(&mutex);
}
