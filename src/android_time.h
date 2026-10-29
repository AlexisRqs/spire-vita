#ifndef ANDROID_TIME_H
#define ANDROID_TIME_H
#include <stdint.h>
typedef struct {int32_t seconds,nanoseconds;} AndroidTimespec;
_Static_assert(sizeof(AndroidTimespec)==8,"Android ARM32 timespec");
int android_clock_gettime(int clock_id,AndroidTimespec *out);
int android_nanosleep(const AndroidTimespec *request,AndroidTimespec *remaining);
uintptr_t android_time_lookup(const char *name);
#endif
