#include "compat_libc.h"
#include <stddef.h>
#include <string.h>
#include <math.h>
int compat_test(uintptr_t (*lookup)(const char *,void *),void *context) {
 /* All calls below use the addresses returned by the supplied resolver. */
 void *(*alloc)(size_t,size_t)=(void *(*)(size_t,size_t))lookup("calloc",context);
 void *(*resize)(void *,size_t)=(void *(*)(void *,size_t))lookup("realloc",context);
 void (*release)(void *)=(void (*)(void *))lookup("free",context);
 void *(*move)(void *,const void *,size_t)=(void *(*)(void *,const void *,size_t))lookup("memmove",context);
 int (*compare)(const char *,const char *)=(int (*)(const char *,const char *))lookup("strcmp",context);
 int (*aligned)(void **,size_t,size_t)=(int (*)(void **,size_t,size_t))lookup("posix_memalign",context);
 double (ANDROID_ABI *power)(double,double)=(double (ANDROID_ABI *)(double,double))lookup("pow",context);
 float (ANDROID_ABI *powerf)(float,float)=(float (ANDROID_ABI *)(float,float))lookup("powf",context);
 double (ANDROID_ABI *split)(double,double *)=(double (ANDROID_ABI *)(double,double *))lookup("modf",context);
 float (ANDROID_ABI *scale)(float,int)=(float (ANDROID_ABI *)(float,int))lookup("ldexpf",context);
 double (ANDROID_ABI *parse)(const char *,char **)=(double (ANDROID_ABI *)(const char *,char **))lookup("strtod",context);
 void (ANDROID_ABI *sc)(double,double *,double *)=(void (ANDROID_ABI *)(double,double *,double *))lookup("sincos",context);
 if (!alloc||!resize||!release||!move||!compare||!aligned||!power||!powerf||!split||!scale||!parse||!sc) return -1;
 void *aligned_data=NULL;
 if(!aligned(&aligned_data,3,32)||aligned_data)return -11;
 if(aligned(&aligned_data,32,64)||!aligned_data)return -12;
 if((uintptr_t)aligned_data%32){release(aligned_data);return -13;}
 release(aligned_data);
 unsigned char *p=alloc(32,1); if(!p)return -2;
 for(unsigned i=0;i<32;i++) if(p[i]){release(p);return -3;}
 memcpy(p,"abcdef",7);
 unsigned char *q=resize(p,64);if(!q){release(p);return -4;}p=q;
 if(compare((char *)p,"abcdef")){release(p);return -5;}
 move(p+1,p,6);
 if(memcmp(p,"aabcdef",7)){release(p);return -6;}release(p);
 if(power(2.0,5.0)!=32.0||powerf(3.0f,3.0f)!=27.0f||scale(1.5f,3)!=12.0f)return -7;
 double integer=0;if(split(3.25,&integer)!=0.25||integer!=3.0)return -8;
 char *end;if(parse("12.5tail",&end)!=12.5||compare(end,"tail"))return -9;
 double s,c;sc(0.5,&s,&c);
 if(fabs(s-0.479425538604203)>1e-12||fabs(c-0.877582561890373)>1e-12)return -10;
 return 0;
}
