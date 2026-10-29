#ifndef ANDROID_PROC_H
#define ANDROID_PROC_H
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
int android_proc_open(const char *path,int flags,...);
ssize_t android_proc_read(int fd,void *out,size_t size);
int android_proc_close(int fd);
uintptr_t android_proc_lookup(const char *name);
#endif
