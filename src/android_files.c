#include "android_files.h"
#include "android_log.h"
#include <stdatomic.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <stdlib.h>
/* Set before guest startup. Android absolute paths are rooted in app data. */
static char root_path[768];
static atomic_uint file_events;
static void trace_file(const char *operation,const char *path,int result){
 int saved=errno;
 if(atomic_fetch_add(&file_events,1)<256)
  android_log_print(result<0?5:4,"SaveIO","%s path=%s result=%d errno=%d",operation,path?path:"(null)",result,result<0?saved:0);
 errno=saved;
}
static int fail(int code){errno=code;return -1;}
static int native_error(void){int e=errno;
 switch(e){case ENOENT:return fail(2);case EACCES:return fail(13);case EEXIST:return fail(17);case ENOTDIR:return fail(20);case EINVAL:return fail(22);case EMFILE:return fail(24);case ENOSPC:return fail(28);default:return fail(5);}}
int android_files_configure(const char *root){
 if(!root||!*root||strlen(root)>=sizeof(root_path))return fail(22);
 strcpy(root_path,root);size_t n=strlen(root_path);while(n>1&&root_path[n-1]=='/')root_path[--n]=0;return 0;
}
int android_file_path(const char *path,char out[1024]){
 if(!path||!out||!root_path[0])return fail(22);
 while(*path=='/')path++;
 if(strchr(path,':')||strchr(path,'\\'))return fail(22);
 const char *s=path;
 while(*s){const char *end=strchr(s,'/');size_t n=end?(size_t)(end-s):strlen(s);
  if(!n||(n==1&&s[0]=='.')||(n==2&&s[0]=='.'&&s[1]=='.'))return fail(22);
  if(!end)break;
  s=end+1;
 }
 if(strlen(root_path)+strlen(path)+2>1024)return fail(36);
 strcpy(out,root_path);if(*path){strcat(out,"/");strcat(out,path);}return 0;
}
static void u32(unsigned char *p,uint32_t v){memcpy(p,&v,4);}
static void u64(unsigned char *p,uint64_t v){memcpy(p,&v,8);}
int android_stat(const char *path,void *out104){
 char full[1024];if(!out104)return fail(14);if(android_file_path(path,full))return -1;
 struct stat s;if(stat(full,&s))return native_error();
 if(s.st_atime>INT32_MAX||s.st_mtime>INT32_MAX||s.st_ctime>INT32_MAX)return fail(75);
 unsigned char b[104]={0};u64(b,s.st_dev);u32(b+12,s.st_ino);u32(b+16,s.st_mode);u32(b+20,s.st_nlink);
 u32(b+24,s.st_uid);u32(b+28,s.st_gid);u64(b+32,s.st_rdev);u64(b+48,s.st_size);
 u32(b+56,s.st_blksize);u64(b+64,s.st_blocks);
 u32(b+72,s.st_atime);u32(b+80,s.st_mtime);u32(b+88,s.st_ctime);u64(b+96,s.st_ino);
 memcpy(out104,b,104);return 0;
}
int android_mkdir(const char *path,unsigned mode){char full[1024];if(android_file_path(path,full))return -1;int result=mkdir(full,mode&0777);if(result)native_error();trace_file("mkdir",path,result);return result;}
FILE *android_fopen(const char *path,const char *mode){char full[1024];if(!mode){fail(22);return NULL;}if(android_file_path(path,full))return NULL;FILE *f=fopen(full,mode);if(!f)native_error();trace_file(mode,path,f?0:-1);return f;}
int android_fseek(FILE *stream,int32_t offset,int whence){
 if(!stream)return fail(22);
 if(fseeko(stream,(off_t)offset,whence))return native_error();
 return 0;
}
int32_t android_ftell(FILE *stream){
 if(!stream)return fail(22);
 off_t position=ftello(stream);if(position<0)return native_error();
 if(position>INT32_MAX)return fail(75);
 return (int32_t)position;
}
int android_remove(const char *path){char full[1024];if(android_file_path(path,full))return -1;if(remove(full))return native_error();return 0;}
/* Android ARM dirent64: inode@0, offset@8, reclen@16, type@18,
 * name@19. VitaSDK struct dirent has a different layout (SceIoStat first). */
typedef struct {DIR *native;char path[1024];uint64_t sequence;unsigned char entry[280];} AndroidDirectory;
void *android_opendir(const char *path){
 char full[1024];if(android_file_path(path,full))return NULL;
 DIR *native=opendir(full);if(!native){native_error();trace_file("opendir",path,-1);return NULL;}
 AndroidDirectory *d=calloc(1,sizeof(*d));if(!d){closedir(native);fail(12);return NULL;}
 d->native=native;strcpy(d->path,full);trace_file("opendir",path,0);return d;
}
void *android_readdir(void *handle){
 AndroidDirectory *d=handle;if(!d){fail(9);return NULL;}
 errno=0;struct dirent *entry=readdir(d->native);
 if(!entry){if(errno)native_error();return NULL;}
 size_t n=strnlen(entry->d_name,256);if(n>255){fail(36);return NULL;}
 char full[1282];int written=snprintf(full,sizeof(full),"%s/%s",d->path,entry->d_name);
 if(written<0||(size_t)written>=sizeof(full)){fail(36);return NULL;}
 struct stat st;if(stat(full,&st)){native_error();return NULL;}
 memset(d->entry,0,sizeof(d->entry));++d->sequence;
 u64(d->entry,d->sequence);u64(d->entry+8,d->sequence);
 uint16_t reclen=(uint16_t)((19+n+1+7)&~7u);memcpy(d->entry+16,&reclen,2);
 d->entry[18]=S_ISDIR(st.st_mode)?4:S_ISREG(st.st_mode)?8:0;
 memcpy(d->entry+19,entry->d_name,n+1);return d->entry;
}
int android_closedir(void *handle){
 AndroidDirectory *d=handle;if(!d)return fail(9);
 int result=closedir(d->native);int saved=errno;free(d);errno=saved;
 return result?native_error():0;
}
uintptr_t android_files_lookup(const char *name){
 if(!strcmp(name,"opendir"))return (uintptr_t)android_opendir;
 if(!strcmp(name,"readdir"))return (uintptr_t)android_readdir;
 if(!strcmp(name,"closedir"))return (uintptr_t)android_closedir;
 if(!strcmp(name,"fseek")||!strcmp(name,"fseeko"))return (uintptr_t)android_fseek;
 if(!strcmp(name,"ftell")||!strcmp(name,"ftello"))return (uintptr_t)android_ftell;
 if(!strcmp(name,"remove"))return (uintptr_t)android_remove;
 if(!strcmp(name,"stat")||!strcmp(name,"stat64"))return (uintptr_t)android_stat;
 if(!strcmp(name,"mkdir"))return (uintptr_t)android_mkdir;
 if(!strcmp(name,"fopen"))return (uintptr_t)android_fopen;
 return 0;
}
