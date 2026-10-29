#ifndef BIONIC_SYNC_H
#define BIONIC_SYNC_H
#include <stdint.h>
uintptr_t bionic_sync_lookup(const char *name);
int bionic_sync_test(uintptr_t (*lookup)(const char *,void *),void *context);
#endif
