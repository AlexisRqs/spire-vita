#include "bionic_local.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/* Guest key IDs and once words are 32 bit, independent of native layouts. */
typedef struct Key {uint32_t id;pthread_key_t native;struct Key *next;} Key;
typedef struct Once {uint32_t *guest;pthread_t owner;struct Once *next;} Once;
static Key *keys;static Once *pending;static uint32_t serial;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static Key *find(uint32_t id){for(Key *k=keys;k;k=k->next)if(k->id==id)return k;return NULL;}
static int key_create(uint32_t *out,void (*destroy)(void *)) {
 if(!out)return 22;
 Key *k=calloc(1,sizeof(*k));if(!k)return 12;
 pthread_mutex_lock(&lock);
 if(serial==UINT32_MAX){pthread_mutex_unlock(&lock);free(k);return 11;}
 int rc=pthread_key_create(&k->native,destroy);
 if(!rc){k->id=++serial;k->next=keys;keys=k;*out=k->id;}
 pthread_mutex_unlock(&lock);if(rc)free(k);return rc?(rc==ENOMEM?12:11):0;
}
static int key_delete(uint32_t id) {
 pthread_mutex_lock(&lock);Key **p=&keys;while(*p&&(*p)->id!=id)p=&(*p)->next;
 if(!*p){pthread_mutex_unlock(&lock);return 22;}
 Key *k=*p;int rc=pthread_key_delete(k->native);
 if(!rc){*p=k->next;free(k);}pthread_mutex_unlock(&lock);return rc?22:0;
}
static void *get(uint32_t id){pthread_mutex_lock(&lock);Key*k=find(id);void*p=k?pthread_getspecific(k->native):NULL;pthread_mutex_unlock(&lock);return p;}
static int set(uint32_t id,const void *value){pthread_mutex_lock(&lock);Key*k=find(id);int rc=k?pthread_setspecific(k->native,value):EINVAL;pthread_mutex_unlock(&lock);return rc?(rc==ENOMEM?12:22):0;}
static void remove_once(Once *o){Once **p=&pending;while(*p!=o)p=&(*p)->next;*p=o->next;free(o);}
static void abandoned(void *ptr){Once *o=ptr;pthread_mutex_lock(&lock);*o->guest=0;remove_once(o);pthread_cond_broadcast(&changed);pthread_mutex_unlock(&lock);}
static int call_once(uint32_t *guest,void (*routine)(void)) {
 if(!guest||!routine||((uintptr_t)guest&3))return 22;
 pthread_mutex_lock(&lock);
 while(*guest==1) {
  Once *o;for(o=pending;o&&o->guest!=guest;o=o->next){}
  if(!o){pthread_mutex_unlock(&lock);return 22;}
  if(pthread_equal(o->owner,pthread_self())){pthread_mutex_unlock(&lock);return 35;}
  pthread_cond_wait(&changed,&lock);
 }
 if(*guest==2){pthread_mutex_unlock(&lock);return 0;}
 if(*guest!=0){pthread_mutex_unlock(&lock);return 22;}
 Once *o=calloc(1,sizeof(*o));if(!o){pthread_mutex_unlock(&lock);return 12;}
 o->guest=guest;o->owner=pthread_self();o->next=pending;pending=o;*guest=1;
 pthread_mutex_unlock(&lock);
 pthread_cleanup_push(abandoned,o);
 routine();
 pthread_cleanup_pop(0);
 pthread_mutex_lock(&lock);*guest=2;remove_once(o);pthread_cond_broadcast(&changed);pthread_mutex_unlock(&lock);return 0;
}
uintptr_t bionic_local_lookup(const char *name) {
#define BIND(n,f) if(!strcmp(name,n))return (uintptr_t)f
 BIND("pthread_key_create",key_create);BIND("pthread_key_delete",key_delete);
 BIND("pthread_getspecific",get);BIND("pthread_setspecific",set);BIND("pthread_once",call_once);
 return 0;
}
