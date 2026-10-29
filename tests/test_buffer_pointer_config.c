#include <assert.h>
#include <stdlib.h>
#include "../src/buffer_pointer.c"
static int fail_symbol=-1,symbol_calls;
int elf_image_symbol(const ElfImage*im,const char*n,uint32_t*v,uint32_t*s){
 (void)im;if(symbol_calls++==fail_symbol)return -1;
 for(unsigned i=0;i<sizeof(contract)/sizeof(*contract);i++)if(!strcmp(n,contract[i].name)){*v=contract[i].value;*s=contract[i].size;return 0;}
 *v=0x3000000+symbol_calls*4;*s=4;return 0;
}
int elf_image_contains(const ElfImage*im,uint32_t a,uint32_t n,uint32_t flags){(void)flags;return a<=im->capacity&&n<=im->capacity-a;}
void *buffer_pointer_original(void*b){return b;}
int android_log_print(int p,const char*t,const char*f,...){(void)p;(void)t;(void)f;return 0;}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);unsigned char*b=calloc(1,0x3100000);assert(b);assert(fread(b,1,0x3100000,f)==0x3100000);fclose(f);
 ElfImage im={.base=b,.capacity=0x3100000,.relocated=1};
 unsigned char entry[8];memcpy(entry,b+(contract[0].value&~1u),8);
 assert(buffer_pointer_configure(NULL)==-1);im.relocated=0;assert(buffer_pointer_configure(&im)==-1);im.relocated=1;
 unsigned count=sizeof(contract)/sizeof(*contract);
 for(unsigned i=0;i<count;i++)for(unsigned j=0;j<contract[i].size;j++){
  unsigned at=(contract[i].value&~1u)+j;b[at]^=1;symbol_calls=0;assert(buffer_pointer_configure(&im)==-1);b[at]^=1;
  assert(!bp_configured);assert(!memcmp(entry,b+(contract[0].value&~1u),8));
 }
 for(fail_symbol=0;fail_symbol<(int)count+7;fail_symbol++){
  symbol_calls=0;assert(buffer_pointer_configure(&im)==-1);assert(!bp_configured);assert(!memcmp(entry,b+(contract[0].value&~1u),8));
 }
 fail_symbol=-1;symbol_calls=0;assert(!buffer_pointer_configure(&im));assert(!atomic_load(&bp_active));
 assert(!memcmp(b+(contract[0].value&~1u),"\xdf\xf8\0\xf0",4));assert(buffer_pointer_configure(&im)==-1);
 buffer_pointer_enable(1);assert(atomic_load(&bp_active));atomic_store(&bp_mismatch,1);buffer_pointer_enable(0);assert(!atomic_load(&bp_active));buffer_pointer_enable(1);assert(!atomic_load(&bp_active));
 free(b);puts("PASS complete binary contract corruption and metadata rejection before patch, startup enable");
}
