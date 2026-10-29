#include <assert.h>
#include <stdlib.h>
#include "../src/gameplay_profile.c"
int grid_objects_configure(ElfImage *im){(void)im;return 0;}
void grid_objects_begin(void *g,void *b,unsigned p,unsigned f,int s){(void)g;(void)b;(void)p;(void)f;(void)s;}
void grid_objects_end(void){}
int grid_card_observer_configure(ElfImage *im){(void)im;return 0;}
void grid_card_observer_begin(unsigned p,unsigned f,int s){(void)p;(void)f;(void)s;}
void grid_card_observer_end(void){}
static int allow_objects,allow_cards,allow_gpu,allow_plain=1;static unsigned replayed,patched;
int grid_objects_plain(void *g,void *b){(void)g;(void)b;return allow_plain;}
int grid_objects_prepare_replay(void *a,void *b,unsigned f,GridBatchPatch *p){(void)a;(void)b;(void)f;(void)p;return allow_objects;}
void grid_batch_apply(unsigned char *b,const GridBatchPatch *p){assert(b==(void*)2);(void)p;patched++;}
int grid_card_observer_can_reuse(unsigned f){(void)f;return allow_cards;}
int graphics_grid_replay_ready(unsigned f){(void)f;return allow_gpu;}
int graphics_grid_replay(unsigned f){(void)f;assert(allow_objects&&allow_cards&&allow_gpu);replayed++;return 1;}
static unsigned trace_samples,flushes;static int flush_ok;
void graphics_grid_trace_begin(unsigned p,int s){(void)p;trace_samples+=s!=0;}
void graphics_grid_trace_end(void){}
void graphics_grid_stream_begin(unsigned p,unsigned f,int s){(void)p;(void)f;(void)s;}
void graphics_grid_stream_end(void){}
int text_submit_flush_batch(void *b){(void)b;flushes++;return flush_ok;}
static int failure=-1;static uint64_t ticks;static unsigned forwarded,logs;
uint64_t scene_clock(void){return ++ticks*10;}
int android_log_print(int p,const char*t,const char*f,...){assert(p==4&&t&&f);logs++;return 0;}
void gameplay_original_0(void*a,void*b,void*c,float d){assert(a==(void*)1&&b==(void*)2&&c==(void*)3&&d==0.375f);forwarded++;}
void *gameplay_original_1(void*a,void*b){assert(a==(void*)1&&b==(void*)2);forwarded++;return (void*)9;}
void *gameplay_original_2(void*a,void*b,void*c,int d){assert(a==(void*)1&&b==(void*)2&&c==(void*)3&&d==1);forwarded++;return (void*)8;}
void gameplay_original_3(void*a,void*b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_4(void*a){assert(a==(void*)1);forwarded++;}
void gameplay_original_5(void*a,void*b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_6(void *a){assert(a==(void*)1);forwarded++;}
void gameplay_original_7(void *a){assert(a==(void*)1);forwarded++;}
void gameplay_original_8(void *a){assert(a==(void*)1);forwarded++;}
void gameplay_original_9(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_10(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_11(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_12(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_13(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_14(void *a,void *b,void *c){assert(a==(void*)1&&b==(void*)2&&c==(void*)3);forwarded++;}
void gameplay_original_15(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_16(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_17(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_18(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_19(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_20(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_21(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_22(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_23(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_24(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_25(void *a,void *b,void *c){assert(a==(void*)1&&b==(void*)2&&c==(void*)3);forwarded++;}
void gameplay_original_26(void *a,void *b){assert(a==(void*)1&&b==(void*)2);forwarded++;}
void gameplay_original_27(void *a,void *b,float c,float d,float e,float f,float g,float h,float j,float k,float l){assert(a==(void*)1&&b==(void*)2&&c==0.25f&&d==-0.5f&&e==1.5f&&f==2.5f&&g==3.5f&&h==4.5f&&j==5.5f&&k==6.5f&&l==7.5f);forwarded++;}
void gameplay_original_28(void *a,void *b,void *c,int d,int e){assert(a==(void*)1&&b==(void*)2&&c==(void*)3&&d==4&&e==5);forwarded++;}
void gameplay_original_29(void *a){assert(a==(void*)1);forwarded++;}
void gameplay_original_30(void*a,void*b,int c,int d,int e,int f){assert(a==(void*)1&&b==(void*)2&&c==4&&d==0&&e==6&&f==1);forwarded++;}
void gameplay_original_31(void*a,void*b,void*c){assert(a==(void*)1&&b==(void*)2&&c==(void*)3);forwarded++;}
void gameplay_original_32(void*a,void*b,void*c){assert(a==(void*)1&&b==(void*)2&&c==(void*)3);forwarded++;}
int elf_image_symbol(const ElfImage*im,const char*n,uint32_t*v,uint32_t*s){(void)im;for(unsigned i=0;i<N;i++)if(!strcmp(n,targets[i].name)){if(failure==(int)i)return -1;*v=targets[i].value;*s=targets[i].size;return 0;}return -1;}
int elf_image_contains(const ElfImage*im,uint32_t a,uint32_t n,uint32_t f){(void)im;(void)a;(void)n;assert(f==5);return 1;}
static void pristine(unsigned char*b){for(unsigned i=0;i<N;i++)assert(!memcmp(b+(targets[i].value&~1u),targets[i].signature,targets[i].length));}
int main(void){
 assert(target_count==27);assert(!gameplay_sprite_profile_enable(1));assert(target_count==N);
 unsigned char*b=calloc(1,0x3100000);assert(b);ElfImage im={.base=b,.relocated=1};
 gameplay_profile_report();assert(!logs);assert(gameplay_profile_configure(NULL)==-1);
 for(unsigned i=0;i<N;i++)memcpy(b+(targets[i].value&~1u),targets[i].signature,targets[i].length);
 for(failure=0;failure<N;failure++){assert(gameplay_profile_configure(&im)==-1);pristine(b);}failure=-1;
 for(unsigned i=0;i<N;i++)for(unsigned j=0;j<targets[i].length;j++){
  unsigned at=(targets[i].value&~1u)+j;b[at]^=1;assert(gameplay_profile_configure(&im)==-1);b[at]^=1;pristine(b);
 }
 assert(!gameplay_profile_configure(&im));assert(gameplay_profile_configure(&im)==-1);assert(gameplay_sprite_profile_enable(0)==-1);
 for(unsigned i=0;i<N;i++){uint32_t at=targets[i].value&~1u,t;memcpy(&t,b+at+4,4);assert(t==(uint32_t)(uintptr_t)targets[i].hook);assert(gameplay_resumes[i]==((uint32_t)(uintptr_t)(b+at+targets[i].length)|1));}
 hook_0((void*)1,(void*)2,(void*)3,0.375f);assert(hook_1((void*)1,(void*)2)==(void*)9);assert(hook_2((void*)1,(void*)2,(void*)3,1)==(void*)8);hook_3((void*)1,(void*)2);hook_4((void*)1);hook_5((void*)1,(void*)2);
 hook_6((void*)1);hook_7((void*)1);hook_8((void*)1);hook_9((void*)1,(void*)2);
 hook_10((void*)1,(void*)2);hook_11((void*)1,(void*)2);hook_12((void*)1,(void*)2);hook_13((void*)1,(void*)2);hook_14((void*)1,(void*)2,(void*)3);
 hook_15((void*)1,(void*)2);hook_16((void*)1,(void*)2);hook_17((void*)1,(void*)2);hook_18((void*)1,(void*)2);hook_19((void*)1,(void*)2);hook_20((void*)1,(void*)2);hook_21((void*)1,(void*)2);hook_22((void*)1,(void*)2);
 hook_23((void*)1,(void*)2);hook_24((void*)1,(void*)2);hook_25((void*)1,(void*)2,(void*)3);hook_26((void*)1,(void*)2);
 hook_27((void*)1,(void*)2,0.25f,-0.5f,1.5f,2.5f,3.5f,4.5f,5.5f,6.5f,7.5f);hook_28((void*)1,(void*)2,(void*)3,4,5);
 hook_29((void*)1);
 hook_30((void*)1,(void*)2,4,0,6,1);hook_31((void*)1,(void*)2,(void*)3);hook_32((void*)1,(void*)2,(void*)3);
 assert(forwarded==N&&logs==3);for(unsigned i=0;i<N;i++)assert(calls[i]==1&&elapsed[i]==10&&maximum[i]==10);
 gameplay_profile_report();assert(logs==N+3);for(unsigned i=0;i<N;i++)assert(!calls[i]&&!elapsed[i]&&!maximum[i]);gameplay_experiment_enable(1);grid_experiment_set(&experiment,4,1);trace_frame=120;flush_ok=1;
 hook_5((void*)1,(void*)2);hook_5((void*)1,(void*)2);assert(trace_samples==2&&flushes==4&&forwarded==N+2);
 grid_experiment_set(&experiment,4,2);flush_ok=0;hook_5((void*)1,(void*)2);
 flush_ok=1;hook_5((void*)1,(void*)2);assert(trace_samples==2&&flushes==8&&forwarded==N+4);
 for(unsigned mask=0;mask<8;mask++){
  allow_objects=mask&1;allow_cards=mask&2;allow_gpu=mask&4;unsigned prev=forwarded,old=replayed;
  grid_experiment_set(&experiment,6,mask);trace_frame=120;flush_ok=1;
  hook_5((void*)1,(void*)2);hook_5((void*)1,(void*)2);
  assert(forwarded==prev+(mask==7?1:2));assert(replayed==old+(mask==7));assert(patched==replayed);assert(experiment.skipped==(mask==7));
 }
 allow_plain=0;unsigned prev=forwarded,old=replayed,old_flush=flushes;grid_experiment_set(&experiment,6,99);hook_5((void*)1,(void*)2);hook_5((void*)1,(void*)2);assert(forwarded==prev+2&&replayed==old&&flushes==old_flush);allow_plain=1;
 gameplay_experiment_enable(2);for(unsigned f=1;f<=2000;f++)gameplay_experiment_present(f);assert(experiment.mode==6);
 gameplay_experiment_enable(1);assert(!experiment.mode&&!replay_default);
 enabled=0;assert(!gameplay_sprite_profile_enable(0));assert(target_count==27);
 for(unsigned i=0;i<N;i++)memcpy(b+(targets[i].value&~1u),targets[i].signature,targets[i].length);
 failure=29;assert(!gameplay_profile_configure(&im));failure=-1;
 for(unsigned i=27;i<N;i++)assert(!memcmp(b+(targets[i].value&~1u),targets[i].signature,targets[i].length));
 for(unsigned i=0;i<27;i++)assert(memcmp(b+(targets[i].value&~1u),targets[i].signature,4));
 free(b);return 0;
}
