#include "android_time.h"
#include <time.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
static int convert(const struct timespec *in,AndroidTimespec *out){
 if(in->tv_sec<INT32_MIN||in->tv_sec>INT32_MAX){errno=75;return -1;}
 if(in->tv_nsec<0||in->tv_nsec>=1000000000L){errno=22;return -1;}
 *out=(AndroidTimespec){(int32_t)in->tv_sec,(int32_t)in->tv_nsec};return 0;
}
int android_clock_gettime(int id,AndroidTimespec *out){
 if(!out){errno=14;return -1;}
 clockid_t native;
 /* Android 0/1 are not VitaSDK's 1/4. */
 if(id==0)native=CLOCK_REALTIME;else if(id==1)native=CLOCK_MONOTONIC;
 else{errno=22;return -1;}
 struct timespec now;
 if(clock_gettime(native,&now)){errno=errno==EINVAL?22:errno==ENOSYS?38:5;return -1;}
 return convert(&now,out);
}
int android_nanosleep(const AndroidTimespec *request,AndroidTimespec *remaining){
 if(!request){errno=14;return -1;}
 if(request->seconds<0||request->nanoseconds<0||request->nanoseconds>=1000000000){errno=22;return -1;}
 struct timespec req={request->seconds,request->nanoseconds},rem={0,0};
 if(!nanosleep(&req,remaining?&rem:NULL))return 0;
 int error=errno;
 if(error==EINTR){if(remaining&&convert(&rem,remaining))return -1;errno=4;}
 else errno=error==EINVAL?22:error==ENOSYS?38:5;
 return -1;
}
uintptr_t android_time_lookup(const char *name){
 if(!strcmp(name,"clock_gettime"))return (uintptr_t)android_clock_gettime;
 if(!strcmp(name,"nanosleep"))return (uintptr_t)android_nanosleep;
 return 0;
}
