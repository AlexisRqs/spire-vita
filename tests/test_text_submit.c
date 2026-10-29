#include <assert.h>
#include <stdlib.h>
#include "../src/text_submit.c"
static int fail=-1;
int android_log_print(int p,const char*t,const char*f,...){(void)p;(void)t;(void)f;return 0;}
int elf_image_symbol(const ElfImage*im,const char*n,uint32_t*v,uint32_t*s){(void)im;
 for(unsigned i=0;i<6;i++)if(!strcmp(n,targets[i].name)){if(fail==(int)i)return -1;*v=targets[i].value;*s=targets[i].size;return 0;}
 if(strstr(n,"cached")){if(fail==6)return -1;*v=16;*s=4;return 0;}
 if(!strcmp(n,"__$gc_stop_threads")){if(fail==7)return -1;*v=20;*s=4;return 0;}return -1;
}
int elf_image_contains(const ElfImage*im,uint32_t a,uint32_t n,uint32_t f){(void)im;(void)a;(void)n;(void)f;return 1;}
int main(void){unsigned char*b=calloc(1,0x3000000);assert(b);ElfImage im={.base=b,.relocated=1};
 assert(text_submit_configure(NULL)==-1);assert(!text_submit_single_page(NULL,NULL));
 for(unsigned i=0;i<6;i++)memcpy(b+(targets[i].value&~1u),targets[i].bytes,targets[i].n);
 for(fail=0;fail<8;fail++){assert(text_submit_configure(&im)==-1);assert(!submit_gc_stop&&!submit_batch_type);for(unsigned i=0;i<6;i++)assert(!submit_expected[i]);}fail=-1;
 for(unsigned i=0;i<6;i++)for(unsigned j=0;j<targets[i].n;j++){unsigned at=(targets[i].value&~1u)+j;b[at]^=1;assert(text_submit_configure(&im)==-1);assert(!submit_gc_stop);b[at]^=1;}
 assert(!text_submit_configure(&im));assert(submit_batch_type==(void*)(b+16)&&submit_gc_stop==(void*)(b+20));
 for(unsigned i=0;i<6;i++)assert(submit_expected[i]==(uintptr_t)(b+targets[i].value));
 free(b);return 0;}
