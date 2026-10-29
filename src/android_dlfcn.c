#include "android_dlfcn.h"
#include <pthread.h>
#include <string.h>
#include <limits.h>
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once=PTHREAD_ONCE_INIT;
static pthread_key_t error_key;
static int error_key_status;
static unsigned references;
static char gles_handle;
static AndroidSymbolLookup resolve;
static void make_key(void){error_key_status=pthread_key_create(&error_key,NULL);}
static void error(const char *s){pthread_once(&once,make_key);if(!error_key_status)pthread_setspecific(error_key,s);}
char *android_dlerror(void){pthread_once(&once,make_key);if(error_key_status)return "dlerror TLS unavailable";char *s=pthread_getspecific(error_key);pthread_setspecific(error_key,NULL);return s;}
void android_dlfcn_configure(AndroidSymbolLookup lookup){pthread_mutex_lock(&lock);resolve=lookup;pthread_mutex_unlock(&lock);}
void *android_dlopen(const char *name,int flags){
 if(!name||strcmp(name,"libGLESv2.so")){error("Unsupported library");return NULL;}
 /* Android accepts legacy zero; reject unsupported mode bits explicitly. */
 if((flags&~0x103)||((flags&3)==3)){error("Unsupported loader flags");return NULL;}
 pthread_mutex_lock(&lock);
 if(!resolve||references==UINT_MAX){pthread_mutex_unlock(&lock);error("GLES loader unavailable");return NULL;}
 references++;pthread_mutex_unlock(&lock);return &gles_handle;
}
void *android_dlsym(void *handle,const char *name){
 pthread_mutex_lock(&lock);AndroidSymbolLookup lookup=resolve;
 int valid=handle==&gles_handle&&references;pthread_mutex_unlock(&lock);
 if(!valid||!name){error("Invalid GLES handle or symbol");return NULL;}
 /* Callback is outside lock. Code remains linked after dlclose. */
 uintptr_t result=lookup(name);
 if(!result)error("GLES symbol unavailable");
 return (void *)result;
}
int android_dlclose(void *handle){
 pthread_mutex_lock(&lock);
 if(handle!=&gles_handle||!references){pthread_mutex_unlock(&lock);error("Invalid GLES handle");return -1;}
 references--;pthread_mutex_unlock(&lock);return 0;
}
uintptr_t android_dlfcn_lookup(const char *name){
#define BIND(n) if(!strcmp(name,#n))return (uintptr_t)android_##n
 BIND(dlopen);BIND(dlsym);BIND(dlclose);BIND(dlerror);
#undef BIND
 return 0;
}
