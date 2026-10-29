#include "compat_libc.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
/* Stable FILE* cells; Android FILE internals remain unsupported. */
static FILE *standard_streams[3];
static pthread_once_t streams_once=PTHREAD_ONCE_INIT;
static void init_streams(void){standard_streams[0]=stdin;standard_streams[1]=stdout;standard_streams[2]=stderr;}
#ifdef __vita__
#include <malloc.h>
#endif
/* Android ARM32 uses base AAPCS for FP arguments/results; VitaSDK uses VFP.
 * Do not map floating point functions directly to Vita libm. */
#define D1(n) static ANDROID_ABI double wrap_##n(double x){return n(x);}
#define F1(n) static ANDROID_ABI float wrap_##n(float x){return n(x);}
#define D2(n) static ANDROID_ABI double wrap_##n(double x,double y){return n(x,y);}
#define F2(n) static ANDROID_ABI float wrap_##n(float x,float y){return n(x,y);}
D1(floor) D1(sin) D1(cos) D1(tan) D1(sinh) D1(cosh) D1(tanh)
D1(acos) D1(asin) D1(atan) D1(exp) D1(log) D1(log10)
D2(fmod) D2(atan2) D2(pow)
F1(ceilf) F1(logf) F1(expf) F1(floorf) F1(acosf) F1(asinf)
F1(log10f) F1(exp2f) F1(cosf) F1(sinf) F1(sinhf)
F2(fmodf) F2(powf) F2(atan2f)
static ANDROID_ABI double wrap_modf(double x,double *p){return modf(x,p);}
static ANDROID_ABI float wrap_ldexpf(float x,int n){return ldexpf(x,n);}
static ANDROID_ABI long wrap_lrintf(float x){return lrintf(x);}
static ANDROID_ABI void wrap_sincos(double x,double *s,double *c){*s=sin(x);*c=cos(x);}
static ANDROID_ABI void wrap_sincosf(float x,float *s,float *c){*s=sinf(x);*c=cosf(x);}
static ANDROID_ABI double wrap_strtod(const char *s,char **end){return strtod(s,end);}
static ANDROID_ABI float wrap_strtof(const char *s,char **end){return strtof(s,end);}
static char *wrap_strcasestr(const char *hay,const char *needle){
 if(!*needle)return (char *)hay;
 for(;*hay;hay++){
  const unsigned char *h=(const unsigned char *)hay,*n=(const unsigned char *)needle;
  while(*h&&*n&&tolower(*h)==tolower(*n)){h++;n++;}
  if(!*n)return (char *)hay;
 }
 return NULL;
}
void *android_memmem(const void *hay,size_t hay_size,const void *needle,size_t needle_size){
 if(!needle_size)return (void *)hay;
 if(needle_size>hay_size)return NULL;
 const unsigned char *h=hay;
 for(size_t i=0;i<=hay_size-needle_size;i++)if(!memcmp(h+i,needle,needle_size))return (void *)(h+i);
 return NULL;
}
static int *wrap_errno(void){return &errno;}
static int wrap_posix_memalign(void **out,size_t alignment,size_t size) {
 if (!out || alignment<sizeof(void *) || (alignment&(alignment-1))) return EINVAL;
#ifdef __vita__
 int saved=errno;
 void *p=memalign(alignment,size);
 errno=saved;
 if (!p && size) return ENOMEM;
 *out=p;return 0;
#else
 return posix_memalign(out,alignment,size);
#endif
}
/* Bionic ARM32 fenv_t is one FPSCR word. Rounding enum is 0..3,
 * unlike host libc ABIs. Keep these adapters ARM-only. */
#if defined(__arm__)
int android_fegetenv(uint32_t *env){
 uint32_t value; __asm__ volatile("vmrs %0, fpscr":"=r"(value));
 *env=value; return 0;
}
int android_fesetenv(const uint32_t *env){
 uint32_t value=*env; __asm__ volatile("vmsr fpscr, %0"::"r"(value)); return 0;
}
int android_fesetround(int rounding){
 if((unsigned)rounding>3)return -1;
 uint32_t value; android_fegetenv(&value);
 value=(value & ~(3u<<22)) | ((uint32_t)rounding<<22);
 return android_fesetenv(&value);
}
#endif
typedef struct { const char *name; uintptr_t address; } Entry;
#define DIRECT(n) {#n,(uintptr_t)n}
#define WRAPPED(n) {#n,(uintptr_t)wrap_##n}
uintptr_t compat_lookup(const char *name) {
 const char *stream_names[]={"stdin","stdout","stderr"};
 for(unsigned i=0;i<3;i++)if(!strcmp(name,stream_names[i])){
  pthread_once(&streams_once,init_streams);return (uintptr_t)&standard_streams[i];
 }
 /* Legacy Android ABI includes sprintf; its caller owns the output capacity.
  * macOS deprecates the symbol, but this table intentionally exposes that ABI. */
#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
 static const Entry entries[]={
#if defined(__arm__)
 {"fegetenv",(uintptr_t)android_fegetenv},
 {"fesetenv",(uintptr_t)android_fesetenv},
 {"fesetround",(uintptr_t)android_fesetround},
#endif
 {"memmem",(uintptr_t)android_memmem},DIRECT(vsnprintf),DIRECT(snprintf),
 DIRECT(sprintf),DIRECT(fprintf),DIRECT(vfprintf),DIRECT(sscanf),DIRECT(vsscanf),
 DIRECT(fgetc),DIRECT(fputc),DIRECT(clearerr),DIRECT(fwrite),DIRECT(fflush),DIRECT(fputs),DIRECT(fread),DIRECT(fclose),DIRECT(feof),DIRECT(ferror),
 DIRECT(malloc),DIRECT(calloc),DIRECT(realloc),DIRECT(free),WRAPPED(posix_memalign),
 DIRECT(memcpy),DIRECT(memmove),DIRECT(memset),DIRECT(memcmp),DIRECT(memchr),
 WRAPPED(strcasestr),DIRECT(sched_yield),DIRECT(usleep),
 DIRECT(strlen),DIRECT(strcpy),DIRECT(strncpy),DIRECT(strcat),DIRECT(strcmp),DIRECT(strncmp),
 DIRECT(strchr),DIRECT(strrchr),DIRECT(strstr),DIRECT(strpbrk),DIRECT(strdup),
 DIRECT(strcasecmp),DIRECT(strncasecmp),DIRECT(strtol),DIRECT(strtoul),DIRECT(strtoll),
 DIRECT(strtoull),DIRECT(atoi),DIRECT(atol),DIRECT(qsort),{"__errno",(uintptr_t)wrap_errno},
 WRAPPED(floor),WRAPPED(sin),WRAPPED(cos),WRAPPED(tan),WRAPPED(sinh),WRAPPED(cosh),WRAPPED(tanh),
 WRAPPED(acos),WRAPPED(asin),WRAPPED(atan),WRAPPED(exp),WRAPPED(log),WRAPPED(log10),
 WRAPPED(fmod),WRAPPED(atan2),WRAPPED(pow),WRAPPED(ceilf),WRAPPED(logf),WRAPPED(expf),
 WRAPPED(floorf),WRAPPED(acosf),WRAPPED(asinf),WRAPPED(log10f),WRAPPED(exp2f),WRAPPED(cosf),
 WRAPPED(sinf),WRAPPED(sinhf),WRAPPED(fmodf),WRAPPED(powf),WRAPPED(atan2f),WRAPPED(modf),
 WRAPPED(ldexpf),WRAPPED(lrintf),WRAPPED(sincos),WRAPPED(sincosf),WRAPPED(strtod),WRAPPED(strtof)
 };
#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic pop
#endif
 for (unsigned i=0;i<sizeof(entries)/sizeof(entries[0]);i++)
  if (!strcmp(entries[i].name,name)) return entries[i].address;
 return 0;
}
