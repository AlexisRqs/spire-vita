#ifndef ANDROID_DLFCN_H
#define ANDROID_DLFCN_H
#include <stdint.h>
typedef uintptr_t (*AndroidSymbolLookup)(const char *);
/* Configure once, before guest threads start. Only built-in GLES is exposed. */
void android_dlfcn_configure(AndroidSymbolLookup lookup);
void *android_dlopen(const char *name,int flags);
void *android_dlsym(void *handle,const char *name);
int android_dlclose(void *handle);
char *android_dlerror(void);
uintptr_t android_dlfcn_lookup(const char *name);
#endif
