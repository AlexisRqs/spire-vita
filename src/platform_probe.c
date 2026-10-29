#include "platform_probe.h"
#include "bionic_threads.h"
#include "android_jump.h"
#include <string.h>
static uint32_t (*thread_self)(void);
static void (*report)(const char *,uint32_t);
static int stack_check(void){
 uintptr_t b=0;size_t n=0;volatile int local=0;
 uint32_t id=thread_self();int rc=bionic_thread_stack(id,&b,&n);
 report("stack thread id",id);report("stack query status",(uint32_t)rc);
 report("stack base",(uint32_t)b);report("stack bytes",(uint32_t)n);
 report("stack local address",(uint32_t)(uintptr_t)&local);
 if(rc)return 1;
 if((uintptr_t)&local<b||(uintptr_t)&local-b>=n)return 2;
 BionicAttr a;
 int (*getattr)(uint32_t,BionicAttr *)=(void *)bionic_threads_lookup("pthread_getattr_np");
 rc=getattr(id,&a);report("stack guest attributes status",(uint32_t)rc);
 if(rc)return 3;
 return a.stack_base!=b||a.stack_size!=n;
}
static void *worker(void *arg){(void)arg;return (void *)(uintptr_t)stack_check();}
int platform_probe(void (*stage)(const char *,uint32_t)){
 report=stage;
 thread_self=(void *)bionic_threads_lookup("pthread_self");
 int rc=stack_check();stage("main stack bounds",rc);int overall=rc?10+rc:0;
 int (*create)(uint32_t *,const BionicAttr *,void *(*)(void *),void *)=(void *)bionic_threads_lookup("pthread_create");
 int (*join)(uint32_t,void **)=(void *)bionic_threads_lookup("pthread_join");
 for(unsigned i=0;i<4;i++){
  uint32_t id;void *result=NULL;
  int created=create(&id,NULL,worker,NULL);stage("worker create status",(uint32_t)created);
  if(created){if(!overall)overall=20;break;}
  int joined=join(id,&result);stage("worker join status",(uint32_t)joined);
  stage("worker stack status",(uint32_t)(uintptr_t)result);
  if(joined)return 21; /* Unknown worker lifetime: do not run more probes. */
  if(result&&!overall)overall=21;
 }
 stage("worker stack batch status",(uint32_t)overall);
 struct {uint32_t before;AndroidJumpBuffer env;uint32_t after;} buffer;
 memset(&buffer,0xa5,sizeof(buffer));
 uint64_t mask=0x123456789abcdefULL,other=0x7777777777777777ULL,out;
 bionic_jump_mask_set(&mask);
 int value=android_setjmp(buffer.env);
 if(!value){bionic_jump_mask_set(&other);android_longjmp(buffer.env,0);}
 if(value!=1||buffer.before!=0xa5a5a5a5||buffer.after!=0xa5a5a5a5)return 30;
 bionic_jump_mask_get(&out);if(out!=mask)return 31;
 value=android__setjmp(buffer.env);
 if(!value){bionic_jump_mask_set(&other);android_siglongjmp(buffer.env,7);}
 bionic_jump_mask_get(&out);if(value!=7||out!=other)return 32;
 mask=0;bionic_jump_mask_set(&mask);stage("jump continuations and software masks",0);return overall;
}
