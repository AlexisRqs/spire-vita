#include "android_jump.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
static pthread_once_t once=PTHREAD_ONCE_INIT;
static pthread_key_t key;
static int failed;
static void init(void){failed=pthread_key_create(&key,free);}
static void ready(void){if(pthread_once(&once,init)||failed)abort();}
void bionic_jump_mask_get(void *out) {
 ready();const void *value=pthread_getspecific(key);uint64_t zero=0;
 memcpy(out,value?value:&zero,sizeof(zero));
}
void bionic_jump_mask_set(const void *in) {
 ready();void *value=pthread_getspecific(key);
 if(!value){value=malloc(8);if(!value)abort();if(pthread_setspecific(key,value)){free(value);abort();}}
 memcpy(value,in,8);
}
void bionic_jump_bad_env(void){abort();}
