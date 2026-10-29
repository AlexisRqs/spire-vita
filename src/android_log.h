#ifndef ANDROID_LOG_H
#define ANDROID_LOG_H
#include <stdint.h>
typedef void (*AndroidLogSink)(int priority,const char *tag,const char *message);
void android_log_set_sink(AndroidLogSink sink);
int android_log_print(int priority,const char *tag,const char *format,...);
uintptr_t android_log_lookup(const char *name);
#endif
