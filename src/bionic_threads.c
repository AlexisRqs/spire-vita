#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "bionic_threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#if defined(__linux__)
#include <unistd.h>
#include <sys/syscall.h>
#endif
#ifdef __vita__
#include <psp2/kernel/threadmgr.h>
#endif
/* Guest IDs are monotonically allocated, never host pointers. Native threads
 * stay joinable internally; a reaper joins guest-detached threads after all
 * native TLS destructors have completed, then releases their records. */
typedef struct Thread {
 uint32_t id;
 pthread_t native;
 void *(*start)(void *); void *arg;
 int detached,finished,joining,adopted,started;
 uint32_t tid;
 struct Thread *next,*pending;
} Thread;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static pthread_once_t once=PTHREAD_ONCE_INIT;
static pthread_key_t current;
static Thread *threads,*queue;
static uint32_t serial;
static int init_error;
static uint32_t native_tid(void) {
#ifdef __vita__
 return (uint32_t)sceKernelGetThreadId();
#elif defined(__APPLE__)
 uint64_t id=0; if(pthread_threadid_np(NULL,&id)||id>INT32_MAX)return 0;return (uint32_t)id;
#elif defined(__linux__)
 return (uint32_t)syscall(SYS_gettid);
#else
 return 0;
#endif
}
static int err(int e){switch(e){case 0:return 0;case EAGAIN:return 11;case ENOMEM:return 12;case EINVAL:return 22;case ESRCH:return 3;case EDEADLK:return 35;case EPERM:return 1;default:return 22;}}
static Thread *find(uint32_t id){for(Thread *t=threads;t;t=t->next)if(t->id==id)return t;return NULL;}
static void erase(Thread *t){Thread **p=&threads;while(*p && *p!=t)p=&(*p)->next;if(*p)*p=t->next;free(t);}
static void enqueue(Thread *t){t->pending=queue;queue=t;pthread_cond_signal(&changed);}
static void finished(void *p) {
 Thread *t=p;pthread_mutex_lock(&lock);
 if(!t->finished){t->finished=1;if(t->adopted)erase(t);else if(t->detached)enqueue(t);}
 pthread_mutex_unlock(&lock);
}
static void *reaper(void *unused) {
 (void)unused;
 for(;;) {
  pthread_mutex_lock(&lock);
  while(!queue)pthread_cond_wait(&changed,&lock);
  Thread *t=queue;queue=t->pending;pthread_mutex_unlock(&lock);
  int rc=pthread_join(t->native,NULL);
  pthread_mutex_lock(&lock);
  if(!rc)erase(t); /* Retain record on unexpected join failure; never free live state. */
  pthread_mutex_unlock(&lock);
 }
 return NULL;
}
static void initialize(void) {
 int rc=pthread_key_create(&current,finished);if(rc){init_error=err(rc);return;}
 pthread_attr_t attr;rc=pthread_attr_init(&attr);
 if(!rc){rc=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
  pthread_t tid;if(!rc)rc=pthread_create(&tid,&attr,reaper,NULL);pthread_attr_destroy(&attr);}
 if(rc){pthread_key_delete(current);init_error=err(rc);}
}
static int ready(void){int rc=pthread_once(&once,initialize);return rc?err(rc):init_error;}
static uint32_t self(void) {
 if(ready())return 0;
 Thread *t=pthread_getspecific(current);if(t)return t->id;
 pthread_t native=pthread_self();pthread_mutex_lock(&lock);
 /* During TLS destruction the key may be cleared before other destructors. */
 for(t=threads;t;t=t->next)if(pthread_equal(t->native,native))break;
 if(!t && serial!=UINT32_MAX) {
  t=calloc(1,sizeof(*t));
  if(t){t->id=++serial;t->native=native;t->tid=native_tid();t->started=1;t->adopted=t->detached=1;t->next=threads;threads=t;
   if(pthread_setspecific(current,t)){erase(t);t=NULL;}}
 }
 uint32_t id=t?t->id:0;pthread_mutex_unlock(&lock);return id;
}
static void *run(void *p) {
 Thread *t=p;
 pthread_mutex_lock(&lock);t->tid=native_tid();t->started=1;pthread_cond_broadcast(&changed);pthread_mutex_unlock(&lock);
 if(pthread_setspecific(current,t)){finished(t);return NULL;}
 return t->start(t->arg);
}
static int attr_init(BionicAttr *a){if(!a)return 22;*a=(BionicAttr){0,0,1024*1024,0,0,0};return 0;}
static int attr_destroy(BionicAttr *a){if(!a)return 22;memset(a,0x42,sizeof(*a));return 0;}
static int attr_detach(BionicAttr *a,int state){if(!a||(state!=0&&state!=1))return 22;a->flags=(a->flags&~1u)|(uint32_t)state;return 0;}
static int attr_stack(BionicAttr *a,uint32_t size){if(!a||size<16384)return 22;a->stack_size=size;return 0;}
static int attr_getstack(const BionicAttr *a,uint32_t *base,uint32_t *size){if(!a||!base||!size)return 22;*base=a->stack_base;*size=a->stack_size;return 0;}
static int create(uint32_t *out,const BionicAttr *guest,void *(*start)(void *),void *arg) {
 if(!out||!start)return 22;
 int rc=ready();if(rc)return rc;
 BionicAttr a;if(guest)a=*guest;else attr_init(&a);
 /* Explicit stacks/guards and scheduling need a platform-specific adapter. */
 if((a.flags&~1u)||a.stack_base||a.guard_size||a.policy||a.priority||a.stack_size<16384)return 22;
 pthread_attr_t native;rc=pthread_attr_init(&native);if(rc)return err(rc);
 rc=pthread_attr_setstacksize(&native,a.stack_size);
 if(rc){pthread_attr_destroy(&native);return err(rc);}
 Thread *t=calloc(1,sizeof(*t));if(!t){pthread_attr_destroy(&native);return 12;}
 pthread_mutex_lock(&lock);
 if(serial==UINT32_MAX){pthread_mutex_unlock(&lock);pthread_attr_destroy(&native);free(t);return 11;}
 t->id=++serial;t->start=start;t->arg=arg;t->detached=(a.flags&1)!=0;t->next=threads;threads=t;
 rc=pthread_create(&t->native,&native,run,t);
 if(rc)erase(t);else *out=t->id;
 pthread_mutex_unlock(&lock);pthread_attr_destroy(&native);return err(rc);
}
static int join(uint32_t id,void **result) {
 uint32_t me=self();if(!me)return 11;if(me==id)return 35;
 pthread_mutex_lock(&lock);Thread *t=find(id);
 if(!t){pthread_mutex_unlock(&lock);return 3;}
 if(t->detached||t->joining){pthread_mutex_unlock(&lock);return 22;}
 t->joining=1;pthread_t native=t->native;pthread_mutex_unlock(&lock);
 int rc=pthread_join(native,result);
 pthread_mutex_lock(&lock);if(!rc)erase(t);else t->joining=0;pthread_mutex_unlock(&lock);return err(rc);
}
static int detach(uint32_t id) {
 if(ready())return 11;
 pthread_mutex_lock(&lock);Thread *t=find(id);int rc=0;
 if(!t)rc=3;else if(t->detached||t->joining)rc=22;
 else {t->detached=1;if(t->finished)enqueue(t);}
 pthread_mutex_unlock(&lock);return rc;
}
static int equal(uint32_t a,uint32_t b){return a==b;}
static void thread_exit(void *result){pthread_exit(result);}
unsigned bionic_threads_live(void){pthread_mutex_lock(&lock);unsigned n=0;for(Thread*t=threads;t;t=t->next)n++;pthread_mutex_unlock(&lock);return n;}

int bionic_thread_stack(uint32_t id,uintptr_t *base,size_t *size) {
 if(!base||!size)return 22;
 if(ready())return 11;
 pthread_mutex_lock(&lock);Thread *t=find(id);
 while(t && !t->started && !t->finished){pthread_cond_wait(&changed,&lock);t=find(id);}
 /* A joiner does not invalidate a live stack. Holding lock prevents the
  * finished TLS destructor from completing, so native join cannot reclaim
  * the handle while this query is in progress. */
 if(!t||t->finished){pthread_mutex_unlock(&lock);return 3;}
 int rc=0;uintptr_t low=0;size_t length=0;
#ifdef __vita__
 SceKernelThreadInfo info;memset(&info,0,sizeof(info));info.size=sizeof(info);
 if(sceKernelGetThreadInfo((SceUID)t->tid,&info)<0||info.stackSize<=0)rc=3;
 else{low=(uintptr_t)info.stack;length=(size_t)info.stackSize;}
#elif defined(__APPLE__)
 uintptr_t top=(uintptr_t)pthread_get_stackaddr_np(t->native);length=pthread_get_stacksize_np(t->native);
 if(top<length)rc=22;else low=top-length;
#elif defined(__linux__)
 pthread_attr_t a;int e=pthread_getattr_np(t->native,&a);
 if(e)rc=err(e);else{void *p=NULL;e=pthread_attr_getstack(&a,&p,&length);pthread_attr_destroy(&a);low=(uintptr_t)p;rc=err(e);}
#else
 rc=38;
#endif
 if(!rc && (!length || length>UINTPTR_MAX-low))rc=22;
 if(!rc && pthread_equal(t->native,pthread_self())) {
  unsigned char marker;uintptr_t here=(uintptr_t)&marker;
  if(here<low || here-low>=length)rc=22;
 }
 if(!rc){*base=low;*size=length;}
 pthread_mutex_unlock(&lock);return rc;
}
int bionic_stack_to_attr(uintptr_t base,size_t size,BionicAttr *out) {
 if(!out||!size)return 22;
 if(base>UINT32_MAX||size>UINT32_MAX||size>UINT32_MAX-base)return 75;
 *out=(BionicAttr){0,(uint32_t)base,(uint32_t)size,0,0,0};return 0;
}
static int getattr(uint32_t id,BionicAttr *out) {
 uintptr_t base;size_t size;int rc=bionic_thread_stack(id,&base,&size);if(rc)return rc;
 rc=bionic_stack_to_attr(base,size,out);if(rc)return rc;
 pthread_mutex_lock(&lock);Thread *t=find(id);if(t)out->flags=t->detached?1:0;else rc=3;pthread_mutex_unlock(&lock);return rc;
}
static int32_t gettid_np(uint32_t id) {
 if(ready())return -1;
 pthread_mutex_lock(&lock);Thread *t=find(id);
 while(t&&!t->started&&!t->finished){pthread_cond_wait(&changed,&lock);t=find(id);}
 int32_t result=t&&!t->finished?(int32_t)t->tid:-1;pthread_mutex_unlock(&lock);return result;
}
static int32_t gettid_current(void){return (int32_t)native_tid();}

uintptr_t bionic_threads_lookup(const char *name) {
#define BIND(n,f) if(!strcmp(name,n))return (uintptr_t)f
 BIND("pthread_attr_init",attr_init);BIND("pthread_attr_destroy",attr_destroy);
 BIND("pthread_attr_setdetachstate",attr_detach);BIND("pthread_attr_setstacksize",attr_stack);
 BIND("pthread_getattr_np",getattr);BIND("pthread_gettid_np",gettid_np);BIND("gettid",gettid_current);
 BIND("pthread_attr_getstack",attr_getstack);BIND("pthread_create",create);
 BIND("pthread_self",self);BIND("pthread_equal",equal);BIND("pthread_join",join);
 BIND("pthread_detach",detach);BIND("pthread_exit",thread_exit);
 return 0;
}
