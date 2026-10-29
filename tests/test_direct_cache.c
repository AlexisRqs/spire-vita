#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../src/direct_cache.c"
uint64_t scene_clock(void){static uint64_t t;return ++t;}
int android_log_print(int prio,const char *tag,const char *fmt,...){(void)prio;(void)tag;(void)fmt;return 0;}
int main(int argc,char **argv){
 assert(argc==5);const char *idx=argv[1],*pack=argv[2],*root=argv[3],*receipt=argv[4];
 remove(receipt);
 assert(!direct_cache_init(idx,pack,receipt));assert(entry_count==2308);
 /* Exercise actual installed dataset and every derived payload. */
 unsigned count=entry_count,eligible=0;
 for(unsigned i=0;i<count;i++){
  const unsigned char *r=index_data+i*RECORD;char p[2048];snprintf(p,sizeof(p),"%s/%s",root,(char*)r);FILE *f=fopen(p,"rb");assert(f);
  size_t n=0;unsigned char *b=direct_cache_read((char*)r,f,&n);
  if(u32(r+284)==0x9276){assert(!b);fclose(f);continue;}eligible++;
  assert(b&&n==u32(r+280)+28);assert(u32(b+36)==u32(r+288)&&u32(b+40)==u32(r+292));assert(u32(b+28)==(u32(r+284)==0x9274?0x83f0:0x83f3));
  assert(u32(b+64)==n-68);free(b);fclose(f);
 }
 assert(metrics.hits==eligible&&metrics.validated==eligible&&!metrics.invalid);
 direct_cache_shutdown();assert(!direct_cache_init(idx,pack,receipt));
 unsigned char *r=index_data;char p[2048];snprintf(p,sizeof(p),"%s/%s",root,(char*)r);FILE *f=fopen(p,"rb");assert(f);size_t n=0;
 unsigned char *b=direct_cache_read((char*)r,f,&n);assert(b&&metrics.validated==0&&metrics.hits==1);free(b);fclose(f);
 f=tmpfile();assert(f);fputs("wrong source",f);n=999;assert(!direct_cache_read((char*)r,f,&n)&&n==999);assert(!direct_cache_read("missing",f,&n));fclose(f);
 /* Inject an invalid same-size source with deliberately changed metadata. */
 f=tmpfile();assert(f);size_t length=u32(r+272);unsigned char *wrong=calloc(length,1);assert(wrong);assert(fwrite(wrong,1,length,f)==length);free(wrong);fflush(f);
 Stamp bad;assert(!stamp(f,&bad));stamps[0]=bad;stamps[0].mtime--;verified[0]=1;
 assert(!direct_cache_read((char*)r,f,&n));assert(!verified[0]);fclose(f);
 /* Check corruption after successful source receipt: never return bad DXT. */
 FILE *real_pack=pack_file;pack_file=tmpfile();assert(pack_file);
 unsigned offset=u32(r+276),bytes=u32(r+280);assert(!fseek(real_pack,offset,SEEK_SET));unsigned char *corrupt=malloc(bytes);assert(corrupt&&fread(corrupt,1,bytes,real_pack)==bytes);corrupt[bytes-1]^=1;
 assert(!fseek(pack_file,offset,SEEK_SET)&&fwrite(corrupt,1,bytes,pack_file)==bytes);fflush(pack_file);free(corrupt);
 f=fopen(p,"rb");assert(f);assert(!direct_cache_read((char*)r,f,&n));fclose(f);fclose(pack_file);pack_file=real_pack;
 direct_cache_report();direct_cache_shutdown();direct_cache_shutdown();
 assert(direct_cache_init("missing",pack,receipt));
 printf("PASS %u actual sources and cached payloads; persistent receipts; fallback; cleanup\n",eligible);return 0;
}
