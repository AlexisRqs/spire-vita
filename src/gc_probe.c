#include "gc_probe.h"
static void *volatile root;
static volatile unsigned finalized,bad_finalizer;
static void finalized_child(void *object,void *tag){
 (void)object;if(tag!=(void *)(uintptr_t)0x1234)bad_finalizer=1;finalized++;
}
__attribute__((noinline)) static int prepare(const GCProbeApi *a){
 uintptr_t *p=a->allocate(64),*c=a->allocate(64);
 if(!p||!c||p==c)return 1;
 p[0]=(uintptr_t)c;p[1]=0x12345678;c[0]=0x76543210;
 root=p;a->finalizer(c,finalized_child,(void *)(uintptr_t)0x1234,NULL,NULL);return 0;
}
__attribute__((noinline)) static int verify(void){
 uintptr_t *p=root;if(!p||p[1]!=0x12345678)return 1;
 uintptr_t *c=(void *)p[0];return !c||c[0]!=0x76543210;
}
/* Overwrite dead frames before the next collection. The collector remains
 * conservative; delayed finalization is reported separately from corruption. */
__attribute__((noinline)) static void scrub(void){
 volatile uintptr_t words[2048];for(unsigned i=0;i<2048;i++)words[i]=0;
 __asm__ volatile("" : : "r"(words) : "memory");
}
int gc_probe_run(const GCProbeApi *a,void (*stage)(const char *,uint32_t)){
 if(!a||!stage||!a->init||!a->allow_threads||!a->allocate||!a->add_roots||!a->finalizer||!a->collect||!a->invoke_finalizers||!a->counter)return 1;
 if(root||finalized)return 2;
 stage("GC_init begin",*a->counter);a->init();stage("GC_init end",*a->counter);
 if(!*a->counter)return 3;
 a->allow_threads();a->add_roots((void *)&root,(char *)&root+sizeof(root));
 if(prepare(a))return 4;
 scrub();uint32_t before=*a->counter;
 stage("rooted collect begin",before);a->collect();a->invoke_finalizers();
 if(*a->counter<=before||finalized||verify())return 5;
 stage("rooted objects intact",*a->counter);root=NULL;
 for(unsigned attempt=0;attempt<8&&!finalized;attempt++){
  scrub();stage("unrooted collect begin",*a->counter);a->collect();a->invoke_finalizers();
 }
 if(!finalized)return 10;
 if(finalized!=1||bad_finalizer)return 6;
 scrub();a->collect();a->invoke_finalizers();
 if(finalized!=1||bad_finalizer)return 7;
 stage("unique finalization confirmed",*a->counter);return 0;
}
