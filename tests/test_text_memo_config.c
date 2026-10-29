#include <assert.h>
#include "../src/text_memo.c"
static int fail=-1,call;
int elf_image_symbol(const ElfImage*im,const char*n,uint32_t*v,uint32_t*s){(void)im;(void)n;int i=call++;*v=128+i*32;if(i<9||i>10)*v|=1;*s=32;return i==fail;}
int elf_image_contains(const ElfImage*im,uint32_t a,uint32_t n,uint32_t f){(void)im;(void)a;(void)n;(void)f;return 1;}
int android_log_print(int p,const char*t,const char*f,...){(void)p;(void)t;(void)f;return 0;}
int main(void){
 assert(memo_active_slots==128&&vertex_active_slots==256);
 assert(!text_cache_capacity_enable(1)&&memo_active_slots==128&&vertex_active_slots==256);
 assert(!text_cache_capacity_enable(0)&&memo_active_slots==128&&vertex_active_slots==256);
 unsigned char data[1024]={0};ElfImage im={.base=data,.relocated=1};
 for(fail=0;fail<13;fail++){call=0;assert(text_memo_configure(&im)<0&&!memo_create);}
 assert(text_memo_configure(NULL)<0);im.relocated=0;assert(text_memo_configure(&im)<0);im.relocated=1;
 fail=-1;call=0;assert(!text_memo_configure(&im));assert((uintptr_t)memo_create==(uintptr_t)(data+129));
 assert(memo_gc_stop==(void*)(data+128+9*32));assert(!memo_rooted);assert(!text_memo_get(NULL,NULL,NULL,NULL));
 assert(text_cache_capacity_enable(1)==-1&&memo_active_slots==128&&vertex_active_slots==256);return 0;}
