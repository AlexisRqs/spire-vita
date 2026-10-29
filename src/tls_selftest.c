#include "android_tls.h"
#include <string.h>

typedef struct {
 AndroidTls tls;
 pthread_mutex_t mutex;
 pthread_cond_t cond;
 int ready, release;
 void *(*resolve)(const AndroidTlsIndex *);
 AndroidTlsIndex index;
 unsigned size;
} Test;
typedef struct { Test *test; unsigned char marker; unsigned char *address; int ok; } Worker;

static unsigned char *address(Test *t) {
 return t->resolve ? t->resolve(&t->index) : android_tls_address(&t->tls,&t->index);
}
static void *worker(void *arg) {
 Worker *w = arg; Test *t = w->test;
 unsigned char *p = address(t);
 w->address = p; w->ok = p != NULL;
 if (p) {
  for (unsigned i = 0; i < t->size; i++) if (p[i]) w->ok = 0;
  if ((uintptr_t)p % 4) w->ok = 0;
  memset(p, w->marker, t->size);
 }
 pthread_mutex_lock(&t->mutex);
 t->ready++; pthread_cond_broadcast(&t->cond);
 while (!t->release) pthread_cond_wait(&t->cond, &t->mutex);
 pthread_mutex_unlock(&t->mutex);
 if (p) {
  if (address(t) != p) w->ok = 0;
  for (unsigned i = 0; i < t->size; i++) if (p[i] != w->marker) w->ok = 0;
 }
 return NULL;
}

static int run_test(void *(*resolve)(const AndroidTlsIndex *),const AndroidTlsIndex *external) {
 Test t; memset(&t, 0, sizeof(t));
 t.resolve=resolve; t.index=external?*external:(AndroidTlsIndex){1,0}; t.size=external?4:20;
 if (!resolve && android_tls_init(&t.tls, NULL, 0, 20, 4)) return -1;
 if (pthread_mutex_init(&t.mutex, NULL)) { android_tls_destroy(&t.tls); return -2; }
 if (pthread_cond_init(&t.cond, NULL)) {
  pthread_mutex_destroy(&t.mutex); android_tls_destroy(&t.tls); return -3;
 }
 AndroidTlsIndex index = {1, 0};
 unsigned char *main_data = address(&t);
 int ok = main_data != NULL;
 if (main_data) memset(main_data, 0x55, t.size);
 for (int round = 0; round < 2; round++) {
  pthread_t threads[4]; Worker workers[4]; int count = 0;
  t.ready = t.release = 0;
  for (int i = 0; i < 4; i++) {
   workers[i] = (Worker){.test=&t, .marker=(unsigned char)(i+1)};
   if (pthread_create(&threads[i], NULL, worker, &workers[i])) { ok = 0; break; }
   count++;
  }
  pthread_mutex_lock(&t.mutex);
  while (t.ready != count) pthread_cond_wait(&t.cond, &t.mutex);
  for (int i = 0; i < count; i++) {
   if (workers[i].address == main_data) ok = 0;
   for (int j = 0; j < i; j++)
    if (workers[i].address == workers[j].address) ok = 0;
  }
  t.release = 1; pthread_cond_broadcast(&t.cond);
  pthread_mutex_unlock(&t.mutex);
  for (int i = 0; i < count; i++) {
   if (pthread_join(threads[i], NULL) || !workers[i].ok) ok = 0;
  }
 }
 if (main_data) for (unsigned i = 0; i < t.size; i++) if (main_data[i] != 0x55) ok = 0;
 if (!resolve) {
  index.offset = 20; if (android_tls_address(&t.tls, &index)) ok = 0;
  index.offset = 0; index.module = 2; if (android_tls_address(&t.tls, &index)) ok = 0;
 }
 if (main_data) memset(main_data,0,t.size);
 pthread_cond_destroy(&t.cond); pthread_mutex_destroy(&t.mutex);
 android_tls_destroy(&t.tls);
 return ok ? 0 : -4;
}
int android_tls_selftest(void) { return run_test(NULL,NULL); }
int android_tls_link_test(void *(*resolve)(const AndroidTlsIndex *),const AndroidTlsIndex *index) {
 if (!resolve || !index) return -5;
 return run_test(resolve,index);
}
