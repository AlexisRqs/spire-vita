#ifndef ANDROID_UTF8_H
#define ANDROID_UTF8_H
#include <stddef.h>
#include <stdint.h>
typedef struct { unsigned char bytes[4]; } AndroidMbState;
size_t android_mbrtowc(uint32_t *out,const char *s,size_t n,AndroidMbState *state);
size_t android_mbrtoc16(uint16_t *out,const char *s,size_t n,AndroidMbState *state);
size_t android_mb_cur_max(void);
size_t android_mbrlen(const char *s,size_t n,AndroidMbState *state);
int android_mbsinit(const AndroidMbState *state);
size_t android_c16rtomb(char *out,uint16_t value,AndroidMbState *state);
size_t android_c32rtomb(char *out,uint32_t value,AndroidMbState *state);
uintptr_t android_utf8_lookup(const char *name);
#endif
