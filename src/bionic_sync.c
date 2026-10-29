#include "bionic_sync.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/* Android ARM32 mutex/cond objects occupy four bytes. Keep native objects
 * separately, keyed by guest address. No native pointer is stored in guest RAM.
 * As with POSIX, destruction requires that no other thread uses the object. */
typedef struct Sync {
 uint32_t *guest;
 int kind;
 union { pthread_mutex_t mutex; pthread_cond_t cond; } native;
 struct Sync *next;
} Sync;
static Sync *objects;
static pthread_mutex_t registry=PTHREAD_MUTEX_INITIALIZER;
static int android_error(int rc) {
 switch(rc) {
 case 0:return 0;case EPERM:return 1;case EAGAIN:return 11;
 case ENOMEM:return 12;case EBUSY:return 16;case EINVAL:return 22;
 case EDEADLK:return 35;case ETIMEDOUT:return 110;
 default:return 22;
 }
}
/* registry must be held. kind: 0 mutex, 1 condition. */
static Sync *find(uint32_t *guest,int kind) {
 for(Sync *s=objects;s;s=s->next)if(s->guest==guest && s->kind==kind)return s;
 return NULL;
}
static int create(uint32_t *guest,int kind,uint32_t state,Sync **out) {
 if(!guest || ((uintptr_t)guest&3))return 22;
 if(kind ? state!=0 : (state!=0 && state!=0x4000 && state!=0x8000))return 22;
 Sync *s=calloc(1,sizeof(*s));if(!s)return 12;
 int rc;
 if(kind)rc=pthread_cond_init(&s->native.cond,NULL);
 else {
  pthread_mutexattr_t attr;rc=pthread_mutexattr_init(&attr);
  if(!rc) {
   int type=state==0x4000?PTHREAD_MUTEX_RECURSIVE:state==0x8000?PTHREAD_MUTEX_ERRORCHECK:PTHREAD_MUTEX_NORMAL;
   rc=pthread_mutexattr_settype(&attr,type);
   if(!rc)rc=pthread_mutex_init(&s->native.mutex,&attr);
   pthread_mutexattr_destroy(&attr);
  }
 }
 if(rc){free(s);return android_error(rc);}
 s->guest=guest;s->kind=kind;s->next=objects;objects=s;*out=s;return 0;
}
static int get(uint32_t *guest,int kind,Sync **out) {
 if(!guest || ((uintptr_t)guest&3))return 22;
 pthread_mutex_lock(&registry);
 *out=find(guest,kind);int rc=*out?0:create(guest,kind,*guest,out);
 pthread_mutex_unlock(&registry);return rc;
}
static int init(uint32_t *guest,int kind,const uint32_t *attr) {
 if(!guest || ((uintptr_t)guest&3))return 22;
 uint32_t state=attr?*attr:0;
 if(!kind){if(state>2)return 22;state<<=14;}
 else if(state)return 22; /* process-shared and monotonic attributes not implemented */
 pthread_mutex_lock(&registry);Sync *s=NULL;
 int rc=find(guest,kind)?16:create(guest,kind,state,&s);
 if(!rc)*guest=state;
 pthread_mutex_unlock(&registry);return rc;
}
static int destroy(uint32_t *guest,int kind) {
 Sync *s;int rc=get(guest,kind,&s);if(rc)return rc;
 pthread_mutex_lock(&registry);
 rc=kind?pthread_cond_destroy(&s->native.cond):pthread_mutex_destroy(&s->native.mutex);
 if(!rc) {
  Sync **link=&objects;while(*link!=s)link=&(*link)->next;
  *link=s->next;*guest=0xdead10cc;free(s);
 }
 pthread_mutex_unlock(&registry);return android_error(rc);
}
static int mutex_init(uint32_t *p,const uint32_t *a){return init(p,0,a);}
static int mutex_destroy(uint32_t *p){return destroy(p,0);}
static int mutex_lock(uint32_t *p){Sync *s;int r=get(p,0,&s);return r?r:android_error(pthread_mutex_lock(&s->native.mutex));}
static int mutex_trylock(uint32_t *p){Sync *s;int r=get(p,0,&s);return r?r:android_error(pthread_mutex_trylock(&s->native.mutex));}
static int mutex_unlock(uint32_t *p){Sync *s;int r=get(p,0,&s);return r?r:android_error(pthread_mutex_unlock(&s->native.mutex));}
static int attr_init(uint32_t *a){if(!a)return 22;*a=0;return 0;}
static int attr_destroy(uint32_t *a){if(!a)return 22;*a=0xffffffff;return 0;}
static int attr_type(uint32_t *a,int type){if(!a||*a>2||type<0||type>2)return 22;*a=(uint32_t)type;return 0;}
static int cond_init(uint32_t *p,const uint32_t *a){return init(p,1,a);}
static int cond_destroy(uint32_t *p){return destroy(p,1);}
static int cond_signal(uint32_t *p){Sync *s;int r=get(p,1,&s);return r?r:android_error(pthread_cond_signal(&s->native.cond));}
static int cond_broadcast(uint32_t *p){Sync *s;int r=get(p,1,&s);return r?r:android_error(pthread_cond_broadcast(&s->native.cond));}
static int cond_wait(uint32_t *p,uint32_t *m) {
 Sync *c,*s;int r=get(p,1,&c);if(r)return r;r=get(m,0,&s);
 return r?r:android_error(pthread_cond_wait(&c->native.cond,&s->native.mutex));
}
typedef struct {int32_t seconds,nanoseconds;} AndroidTimespec;
static int cond_timedwait(uint32_t *p,uint32_t *m,const AndroidTimespec *deadline) {
 if(!deadline || deadline->nanoseconds<0 || deadline->nanoseconds>=1000000000)return 22;
 struct timespec native={deadline->seconds,deadline->nanoseconds};
 Sync *c,*s;int r=get(p,1,&c);if(r)return r;r=get(m,0,&s);
 return r?r:android_error(pthread_cond_timedwait(&c->native.cond,&s->native.mutex,&native));
}
uintptr_t bionic_sync_lookup(const char *name) {
#define BIND(n,f) if(!strcmp(name,n))return (uintptr_t)f
 BIND("pthread_mutex_init",mutex_init);BIND("pthread_mutex_destroy",mutex_destroy);
 BIND("pthread_mutex_lock",mutex_lock);BIND("pthread_mutex_trylock",mutex_trylock);
 BIND("pthread_mutex_unlock",mutex_unlock);BIND("pthread_mutexattr_init",attr_init);
 BIND("pthread_mutexattr_destroy",attr_destroy);BIND("pthread_mutexattr_settype",attr_type);
 BIND("pthread_cond_init",cond_init);BIND("pthread_cond_destroy",cond_destroy);
 BIND("pthread_cond_signal",cond_signal);BIND("pthread_cond_broadcast",cond_broadcast);
 BIND("pthread_cond_wait",cond_wait);BIND("pthread_cond_timedwait",cond_timedwait);
 return 0;
}
