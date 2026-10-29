#ifndef COMPAT_LIBC_H
#define COMPAT_LIBC_H
#include <stdint.h>
#if defined(__arm__)
#define ANDROID_ABI __attribute__((pcs("aapcs")))
#else
#define ANDROID_ABI
#endif
uintptr_t compat_lookup(const char *name);
int compat_test(uintptr_t (*lookup)(const char *,void *),void *context);
#endif
