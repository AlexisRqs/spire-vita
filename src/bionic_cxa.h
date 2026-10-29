#ifndef BIONIC_CXA_H
#define BIONIC_CXA_H
#include <stdint.h>
int bionic_cxa_atexit(void (*destructor)(void *),void *argument,void *dso);
void bionic_cxa_finalize(void *dso);
uintptr_t bionic_cxa_lookup(const char *name);
#endif
