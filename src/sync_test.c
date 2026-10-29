#include "bionic_sync.h"
#include <pthread.h>
#include <string.h>
typedef struct {uint32_t before,value,after;} Guest;
typedef struct {
 int (*lock)(uint32_t *),(*unlock)(uint32_t *),(*signal)(uint32_t *),(*broadcast)(uint32_t *);
 int (*wait)(uint32_t *,uint32_t *);
 Guest mutex,cond;
 int ready,release,total;
} Shared;
typedef struct {Shared *s;int error;} Worker;
static void *run(void *arg) {
 Worker *w=arg;Shared *s=w->s;
 if(s->lock(&s->mutex.value)){w->error=1;return NULL;}
 s->ready++;s->broadcast(&s->cond.value);
 while(!s->release)if(s->wait(&s->cond.value,&s->mutex.value)){w->error=2;break;}
 s->unlock(&s->mutex.value);
 for(int i=0;i<500;i++) {
  if(s->lock(&s->mutex.value)){w->error=3;break;}
  s->total++;
  if(s->unlock(&s->mutex.value)){w->error=4;break;}
 }
 return NULL;
}
int bionic_sync_test(uintptr_t (*lookup)(const char *,void *),void *context) {
#define OP(n) ((int (*)(uint32_t *))lookup(n,context))
 int (*init_m)(uint32_t *,const uint32_t *)=(int (*)(uint32_t *,const uint32_t *))lookup("pthread_mutex_init",context);
 int (*init_c)(uint32_t *,const uint32_t *)=(int (*)(uint32_t *,const uint32_t *))lookup("pthread_cond_init",context);
 int (*destroy_m)(uint32_t *)=OP("pthread_mutex_destroy"),(*destroy_c)(uint32_t *)=OP("pthread_cond_destroy");
 int (*trylock)(uint32_t *)=OP("pthread_mutex_trylock");
 int (*attr_init)(uint32_t *)=OP("pthread_mutexattr_init"),(*attr_destroy)(uint32_t *)=OP("pthread_mutexattr_destroy");
 int (*attr_type)(uint32_t *,int)=(int (*)(uint32_t *,int))lookup("pthread_mutexattr_settype",context);
 Shared s;memset(&s,0,sizeof(s));
 s.mutex=(Guest){0x12345678,0,0x87654321};s.cond=s.mutex;
 s.lock=OP("pthread_mutex_lock");s.unlock=OP("pthread_mutex_unlock");
 s.signal=OP("pthread_cond_signal");s.broadcast=OP("pthread_cond_broadcast");
 s.wait=(int (*)(uint32_t *,uint32_t *))lookup("pthread_cond_wait",context);
 if(!init_m||!init_c||!destroy_m||!destroy_c||!trylock||!attr_init||!attr_destroy||!attr_type||!s.lock||!s.unlock||!s.signal||!s.broadcast||!s.wait)return -1;
 Guest recursive={0x12345678,0x4000,0x87654321},attr={0x12345678,0,0x87654321},checked=attr;
 if(s.lock(&recursive.value)||s.lock(&recursive.value)||s.unlock(&recursive.value)||s.unlock(&recursive.value)||destroy_m(&recursive.value))return -2;
 if(attr_init(&attr.value)||attr_type(&attr.value,2)||init_m(&checked.value,&attr.value)||attr_destroy(&attr.value))return -3;
 if(s.lock(&checked.value))return -4;
 int deadlock=s.lock(&checked.value);
 if(s.unlock(&checked.value)||destroy_m(&checked.value)||deadlock!=35)return -5;
 if(init_c(&s.cond.value,NULL))return -6;
 pthread_t threads[2];Worker workers[2];int count=0,result=0;
 for(int i=0;i<2;i++) {
  workers[i]=(Worker){&s,0};
  if(pthread_create(&threads[i],NULL,run,&workers[i])){result=-7;break;}count++;
 }
 if(s.lock(&s.mutex.value))return -8;
 while(s.ready<count)s.wait(&s.cond.value,&s.mutex.value);
 if(trylock(&s.mutex.value)!=16)result=-9;
 s.release=1;s.broadcast(&s.cond.value);s.unlock(&s.mutex.value);
 for(int i=0;i<count;i++)if(pthread_join(threads[i],NULL)||workers[i].error)result=-10;
 if(s.total!=count*500)result=-11;
 if(s.signal(&s.cond.value)||destroy_c(&s.cond.value)||destroy_m(&s.mutex.value))result=-12;
 Guest *guards[]={&s.mutex,&s.cond,&recursive,&attr,&checked};
 for(unsigned i=0;i<sizeof(guards)/sizeof(guards[0]);i++)
  if(guards[i]->before!=0x12345678||guards[i]->after!=0x87654321)result=-13;
 return result;
}
