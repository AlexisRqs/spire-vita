#ifndef ANDROID_JUMP_H
#define ANDROID_JUMP_H
#include <stdint.h>
/* Private bridge format in Android ARM32's 256-byte public buffer.
 * Jump buffers must remain on their originating thread and live stack. */
typedef uint32_t AndroidJumpBuffer[64];
int android_setjmp(AndroidJumpBuffer env) __attribute__((returns_twice));
int android__setjmp(AndroidJumpBuffer env) __attribute__((returns_twice));
int android_sigsetjmp(AndroidJumpBuffer env,int save_mask) __attribute__((returns_twice));
void android_longjmp(AndroidJumpBuffer env,int value) __attribute__((noreturn));
void android_siglongjmp(AndroidJumpBuffer env,int value) __attribute__((noreturn));
/* Software mask for the Android compatibility layer, separate from Vita
 * kernel exception delivery. No Linux signal delivery is claimed here. */
void bionic_jump_mask_get(void *eight_bytes);
void bionic_jump_mask_set(const void *eight_bytes);
void bionic_jump_bad_env(void) __attribute__((noreturn));
#endif
