#ifndef ANDROID_FILES_H
#define ANDROID_FILES_H
#include <stdint.h>
#include <stdio.h>
int android_files_configure(const char *root);
int android_file_path(const char *path,char out[1024]);
int android_stat(const char *path,void *out104);
int android_mkdir(const char *path,unsigned mode);
FILE *android_fopen(const char *path,const char *mode);
int android_fseek(FILE *stream,int32_t offset,int whence);
int32_t android_ftell(FILE *stream);
int android_remove(const char *path);
void *android_opendir(const char *path);
void *android_readdir(void *handle);
int android_closedir(void *handle);
uintptr_t android_files_lookup(const char *name);
#endif
