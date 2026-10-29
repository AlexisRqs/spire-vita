#include "android_log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static AndroidLogSink output;
void android_log_set_sink(AndroidLogSink sink){pthread_mutex_lock(&lock);output=sink;pthread_mutex_unlock(&lock);}
int android_log_print(int priority,const char *tag,const char *format,...){
 if(!format)return -1;
 char message[2048];va_list args;va_start(args,format);
 int n=vsnprintf(message,sizeof(message),format,args);va_end(args);
 if(n<0)return -1;
 pthread_mutex_lock(&lock);AndroidLogSink sink=output;pthread_mutex_unlock(&lock);
 if(sink)sink(priority,tag?tag:"",message);
 return n<(int)sizeof(message)?n:(int)sizeof(message)-1;
}
/* Bounded diagnostics: never allocate while reporting memory pressure. */
int android_diagnostic_printf(const char *format,...){
 if(!format){errno=EINVAL;return -1;}
 char message[2048];va_list args;va_start(args,format);
 int n=vsnprintf(message,sizeof(message),format,args);va_end(args);
 if(n<0)return -1;
 pthread_mutex_lock(&lock);AndroidLogSink sink=output;pthread_mutex_unlock(&lock);
 if(sink)sink(4,"stdout",message);
 if(n>=(int)sizeof(message)){errno=EOVERFLOW;return -1;}
 return n;
}
/* Guest fd 1/2 are diagnostic streams, not host-owned file descriptors.
 * Return short writes, preserving write(2) retry semantics used by BDW GC. */
static ssize_t android_diagnostic_write(int fd,const void *data,size_t size){
 if(fd!=1&&fd!=2){errno=EBADF;return -1;}
 if(!size)return 0;
 if(!data){errno=EFAULT;return -1;}
 size_t n=size>1023?1023:size;
 char chunk[1024];memcpy(chunk,data,n);chunk[n]=0;
 pthread_mutex_lock(&lock);AndroidLogSink sink=output;pthread_mutex_unlock(&lock);
 if(sink)sink(fd==2?5:4,fd==2?"stderr":"stdout",chunk);
 return (ssize_t)n;
}
int android_diagnostic_puts(const char *text){
 if(!text){errno=EINVAL;return -1;}
 size_t size=strlen(text);if(size>=INT_MAX){errno=EOVERFLOW;return -1;}
 for(size_t pos=0;pos<size;){ssize_t n=android_diagnostic_write(1,text+pos,size-pos);if(n<=0)return -1;pos+=(size_t)n;}
 if(android_diagnostic_write(1,"\n",1)!=1)return -1;
 return (int)size+1;
}
/* Bionic ARM32 __sF has three 84-byte FILE records. These are opaque
 * identity tokens only: never pass their incompatible layout to newlib. */
static _Alignas(8) unsigned char bionic_streams[3*84];
static int diagnostic_stream(FILE *f){
 if((void *)f==bionic_streams+84)return 1;
 if((void *)f==bionic_streams+168)return 2;
 return 0;
}
static FILE *native_stream(FILE *f){
 if((void *)f==bionic_streams)return stdin;
 int fd=diagnostic_stream(f);return fd==1?stdout:fd==2?stderr:f;
}
static size_t diagnostic_fwrite(const void *p,size_t size,size_t count,FILE *f){
 int fd=diagnostic_stream(f);if(!fd){size_t n=fwrite(p,size,count,native_stream(f));
  if(n!=count){int saved=errno;android_log_print(5,"SaveIO","short fwrite items=%u/%u errno=%d",(unsigned)n,(unsigned)count,saved);errno=saved;}return n;}
 if(!size||!count)return 0;
 if(!p){errno=EFAULT;return 0;}
 if(count>SIZE_MAX/size){errno=EOVERFLOW;return 0;}
 size_t total=size*count,pos=0;
 while(pos<total){ssize_t n=android_diagnostic_write(fd,(const char *)p+pos,total-pos);if(n<=0)break;pos+=(size_t)n;}
 return pos/size;
}
static int diagnostic_vfprintf(FILE *f,const char *format,va_list args){
 int fd=diagnostic_stream(f);if(!fd)return vfprintf(native_stream(f),format,args);
 if(!format){errno=EINVAL;return -1;}
 char message[2048];int n=vsnprintf(message,sizeof(message),format,args);
 if(n<0)return n;
 size_t bytes=n<(int)sizeof(message)?(size_t)n:sizeof(message)-1;
 if(diagnostic_fwrite(message,1,bytes,f)!=bytes)return -1;
 if(n>=(int)sizeof(message)){errno=EOVERFLOW;return -1;}
 return n;
}
static int diagnostic_fprintf(FILE *f,const char *format,...){
 va_list args;va_start(args,format);int n=diagnostic_vfprintf(f,format,args);va_end(args);return n;
}
static int diagnostic_fputs(const char *s,FILE *f){
 if(!diagnostic_stream(f))return fputs(s,native_stream(f));
 size_t n=strlen(s);return diagnostic_fwrite(s,1,n,f)==n?0:EOF;
}
static void diagnostic_perror(const char *prefix){
 int saved=errno;
 android_log_print(5,"FileError","%s%sAndroid errno=%d",prefix?prefix:"",prefix&&*prefix?": ":"",saved);
 errno=saved;
}
static int diagnostic_setvbuf(FILE *f,char *buffer,int mode,size_t size){
 /* Bionic uses 0/1/2; translate instead of assuming the host enum. */
 if(!f||mode<0||mode>2){errno=22;return -1;}
 if(diagnostic_stream(f))return 0;
 int modes[]={_IOFBF,_IOLBF,_IONBF};
 return setvbuf(native_stream(f),buffer,modes[mode],size);
}
static int diagnostic_fflush(FILE *f){return diagnostic_stream(f)?0:fflush(native_stream(f));}
static int diagnostic_fputc(int c,FILE *f){unsigned char b=(unsigned char)c;return diagnostic_stream(f)?(diagnostic_fwrite(&b,1,1,f)==1?b:EOF):fputc(c,native_stream(f));}
static int diagnostic_fgetc(FILE *f){return fgetc(native_stream(f));}
static size_t diagnostic_fread(void *p,size_t s,size_t n,FILE *f){return fread(p,s,n,native_stream(f));}
static int diagnostic_fclose(FILE *f){if((void*)f==bionic_streams||diagnostic_stream(f)){errno=EBADF;return EOF;}int result=fclose(f);if(result){int saved=errno;android_log_print(5,"SaveIO","fclose failed errno=%d",saved);errno=saved;}return result;}
static int diagnostic_feof(FILE *f){return feof(native_stream(f));}
static int diagnostic_ferror(FILE *f){return ferror(native_stream(f));}
static void diagnostic_clearerr(FILE *f){clearerr(native_stream(f));}
uintptr_t android_log_lookup(const char *name){
 if(!strcmp(name,"__sF"))return (uintptr_t)bionic_streams;
 #define OUTPUT(n) if(!strcmp(name,#n))return (uintptr_t)diagnostic_##n
 OUTPUT(perror);OUTPUT(setvbuf);OUTPUT(fwrite);OUTPUT(vfprintf);OUTPUT(fprintf);OUTPUT(fputs);OUTPUT(fflush);
 OUTPUT(fputc);OUTPUT(fgetc);OUTPUT(fread);OUTPUT(fclose);OUTPUT(feof);OUTPUT(ferror);OUTPUT(clearerr);
 #undef OUTPUT
 if(!strcmp(name,"puts"))return (uintptr_t)android_diagnostic_puts;
 if(!strcmp(name,"printf"))return (uintptr_t)android_diagnostic_printf;
 if(!strcmp(name,"write"))return (uintptr_t)android_diagnostic_write;
 return !strcmp(name,"__android_log_print")?(uintptr_t)android_log_print:0;
}
