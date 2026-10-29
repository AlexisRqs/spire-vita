#include "android_proc.h"
#include <pthread.h>
#include <string.h>
#include <errno.h>
/* Read-only platform descriptions consumed by the bundled NDK cpufeatures.
 * Cortex-A9: ARMv7, VFPv3 and NEON. No invented Linux host files exposed. */
static const char cpuinfo[]="Processor\t: ARMv7 Processor\nCPU architecture: 7\nFeatures\t: vfp vfpv3 neon\nCPU implementer\t: 0x41\nCPU part\t: 0xc09\nHardware\t: PS Vita\n";
static const char cpus[]="0-3\n";
static struct {const char *bytes;size_t offset;} slots[8];
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
int android_proc_open(const char *path,int flags,...){
 if(!path){errno=EFAULT;return -1;}
 const char *bytes=NULL;
 if(!strcmp(path,"/proc/cpuinfo"))bytes=cpuinfo;
 if(!strcmp(path,"/sys/devices/system/cpu/possible")||!strcmp(path,"/sys/devices/system/cpu/present"))bytes=cpus;
 if(!bytes){errno=ENOENT;return -1;}
 if(flags&~0x80000){errno=EACCES;return -1;} /* O_RDONLY plus Android O_CLOEXEC */
 pthread_mutex_lock(&lock);
 for(unsigned i=0;i<8;i++)if(!slots[i].bytes){slots[i].bytes=bytes;slots[i].offset=0;pthread_mutex_unlock(&lock);return 0x4000+(int)i;}
 pthread_mutex_unlock(&lock);errno=EMFILE;return -1;
}
ssize_t android_proc_read(int fd,void *out,size_t size){
 if(fd<0x4000||fd>=0x4008){errno=EBADF;return -1;}
 pthread_mutex_lock(&lock);unsigned i=(unsigned)(fd-0x4000);
 if(!slots[i].bytes){pthread_mutex_unlock(&lock);errno=EBADF;return -1;}
 if(!out&&size){pthread_mutex_unlock(&lock);errno=EFAULT;return -1;}
 size_t left=strlen(slots[i].bytes)-slots[i].offset;if(size>left)size=left;
 if(size)memcpy(out,slots[i].bytes+slots[i].offset,size);
 slots[i].offset+=size;pthread_mutex_unlock(&lock);return (ssize_t)size;
}
int android_proc_close(int fd){
 if(fd<0x4000||fd>=0x4008){errno=EBADF;return -1;}
 pthread_mutex_lock(&lock);unsigned i=(unsigned)(fd-0x4000);
 if(!slots[i].bytes){pthread_mutex_unlock(&lock);errno=EBADF;return -1;}
 slots[i].bytes=NULL;slots[i].offset=0;pthread_mutex_unlock(&lock);return 0;
}
uintptr_t android_proc_lookup(const char *name){
 if(!strcmp(name,"open"))return (uintptr_t)android_proc_open;
 if(!strcmp(name,"read"))return (uintptr_t)android_proc_read;
 if(!strcmp(name,"close"))return (uintptr_t)android_proc_close;
 return 0;
}
