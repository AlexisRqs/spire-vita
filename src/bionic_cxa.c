#include "bionic_cxa.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
typedef struct ExitEntry {
 void (*function)(void *);
 void *argument,*dso;
 struct ExitEntry *next;
} ExitEntry;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static ExitEntry *entries;
int bionic_cxa_atexit(void (*function)(void *),void *argument,void *dso){
 if(!function)return -1;
 ExitEntry *e=malloc(sizeof(*e));if(!e)return -1;
 *e=(ExitEntry){function,argument,dso,NULL};
 pthread_mutex_lock(&lock);e->next=entries;entries=e;pthread_mutex_unlock(&lock);
 return 0;
}
void bionic_cxa_finalize(void *dso){
 /* Remove before calling: a recursive/concurrent finalizer cannot claim this
  * entry twice. Callbacks may register another destructor or finalize a DSO.
  * Image unload must be serialized by the owner and await all callbacks. */
 for(;;){
  pthread_mutex_lock(&lock);ExitEntry **p=&entries;
  while(*p&&dso&&(*p)->dso!=dso)p=&(*p)->next;
  ExitEntry *e=*p;if(e)*p=e->next;pthread_mutex_unlock(&lock);
  if(!e)return;
  void (*function)(void *)=e->function;void *argument=e->argument;
  free(e);function(argument);
 }
}
uintptr_t bionic_cxa_lookup(const char *name){
 if(!strcmp(name,"__cxa_atexit"))return (uintptr_t)bionic_cxa_atexit;
 if(!strcmp(name,"__cxa_finalize"))return (uintptr_t)bionic_cxa_finalize;
 return 0;
}
