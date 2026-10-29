#ifndef ANDROID_FRAME_LOOP_H
#define ANDROID_FRAME_LOOP_H
#include <stdint.h>
/* Deliberately limited to the port's main-thread frame wake source.
 * No Android fd registrations/callbacks or general-purpose looper emulation. */
typedef int (*AndroidFramePump)(int timeout_ms,void *context);
int android_frame_loop_configure(AndroidFramePump pump,void *context);
int android_frame_poll(int timeout_ms,int *fd,int *events,void **data);
uintptr_t android_frame_loop_lookup(const char *name);
#endif
