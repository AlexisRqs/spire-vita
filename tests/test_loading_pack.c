#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static int allocation_failure=-1,read_failure;
static void *test_malloc(size_t n){if(allocation_failure==0)return NULL;if(allocation_failure>0)allocation_failure--;return malloc(n);}
static void *test_calloc(size_t n,size_t z){if(allocation_failure==0)return NULL;if(allocation_failure>0)allocation_failure--;return calloc(n,z);}
static size_t test_fread(void *b,size_t n,size_t z,FILE*f){return read_failure?0:fread(b,n,z,f);}
#define malloc test_malloc
#define calloc test_calloc
#define fread test_fread
#include "../src/direct_cache.c"
#undef malloc
#undef calloc
#undef fread
uint64_t scene_clock(void){static uint64_t t;return ++t;}
int android_log_print(int p,const char*t,const char*f,...){(void)p;(void)t;(void)f;return 0;}
static void write_file(const char *p,const void *b,size_t n){FILE*f=fopen(p,"wb");assert(f);assert(fwrite(b,1,n,f)==n);assert(!fclose(f));}
int main(int argc,char **argv){
 assert(argc==7);const char *index=argv[1],*pack=argv[2],*root=argv[3],*receipt=argv[4],*bundle=argv[5],*scratch=argv[6];
 assert(!direct_cache_init(index,pack,receipt));
 direct_cache_loading_begin();assert(!loading_data);
 direct_cache_loading_path(bundle);
 for(int k=0;k<2;k++){allocation_failure=k;loading_attempted=0;direct_cache_loading_begin();assert(!loading_data&&!loading_entries);}
 allocation_failure=-1;read_failure=1;loading_attempted=0;direct_cache_loading_begin();assert(!loading_data&&!loading_entries);read_failure=0;loading_attempted=0;
 direct_cache_loading_begin();assert(loading_data);
 unsigned count=0;size_t total=u32(loading_data+20);unsigned char *saved=malloc(total);assert(saved);memcpy(saved,loading_data,total);
 for(unsigned i=0;i<entry_count;i++)if(loading_entries[i]){
  count++;const unsigned char*r=index_data+i*RECORD;char path[2048];snprintf(path,sizeof path,"%s/%s",root,r);FILE*f=fopen(path,"rb");assert(f);
  size_t n,m;unsigned char *a=direct_cache_read((const char*)r,f,&n);assert(a);const unsigned char **keep=loading_entries;loading_entries=NULL;
  unsigned char *b=direct_cache_read((const char*)r,f,&m);loading_entries=keep;assert(b&&n==m&&!memcmp(a,b,n));free(a);free(b);fclose(f);
 }
 assert(count==486&&loading_reads==486);
 /* Source validation still runs before using prefetched bytes. */
 unsigned i=0;while(!loading_entries[i])i++;FILE *bad=tmpfile();assert(bad);fputs("bad source",bad);fflush(bad);size_t n=777;
 assert(!direct_cache_read((char*)index_data+i*RECORD,bad,&n)&&n==777);fclose(bad);
 direct_cache_loading_end();assert(!loading_data&&!loading_entries);
 direct_cache_loading_begin();assert(!loading_data&&loading_reads==0);
 direct_cache_loading_path(scratch);
 /* Wrong magic/version/dataset/count/length, bad index/size/payload, duplicates,
  * truncation and trailing bytes all reject atomically. */
 size_t locations[]={0,4,8,16,20,24,28,total-1};
 for(unsigned k=0;k<sizeof locations/sizeof *locations;k++){
  saved[locations[k]]^=0x80;write_file(scratch,saved,total);loading_attempted=0;direct_cache_loading_begin();assert(!loading_data&&!loading_entries);saved[locations[k]]^=0x80;
 }
 size_t second=24+8+u32(saved+28);unsigned old=u32(saved+second);put32(saved+second,u32(saved+24));write_file(scratch,saved,total);loading_attempted=0;direct_cache_loading_begin();assert(!loading_data);put32(saved+second,old);
 write_file(scratch,saved,total-1);loading_attempted=0;direct_cache_loading_begin();assert(!loading_data);
 write_file(scratch,saved,12);loading_attempted=0;direct_cache_loading_begin();assert(!loading_data);
 write_file(scratch,saved,total);FILE*f=fopen(scratch,"ab");assert(f);fputc(0,f);fclose(f);loading_attempted=0;direct_cache_loading_begin();assert(!loading_data);
 direct_cache_loading_path("missing");loading_attempted=0;direct_cache_loading_begin();assert(!loading_data);
 /* Repeat startup/reload lifetime; disabled path never retains memory. */
 for(unsigned k=0;k<3;k++){assert(!direct_cache_init(index,pack,receipt));direct_cache_loading_path(bundle);direct_cache_loading_begin();assert(loading_data);direct_cache_loading_end();assert(!loading_data);}
 assert(!direct_cache_init(index,pack,receipt));direct_cache_loading_path(bundle);direct_cache_loading_begin();direct_cache_shutdown();assert(!loading_data&&!loading_entries);
 direct_cache_loading_path(NULL);assert(!direct_cache_init(index,pack,receipt));direct_cache_loading_begin();assert(!loading_data);direct_cache_shutdown();free(saved);remove(scratch);
 printf("PASS %u byte-identical outputs; %u pack reads avoided; source mismatch, atomic bundle refusal, lifetime and disable fallback\n",count,count);
}
