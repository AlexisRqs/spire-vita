#include "android_files.h"
#include "android_log.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
static uint32_t word(const unsigned char *p){uint32_t v;memcpy(&v,p,4);return v;}
int main(void){char root[]="/tmp/spire-files-XXXXXX",full[1024];assert(mkdtemp(root));assert(!android_files_configure(root));
 assert(android_file_path("../bad",full)==-1);assert(android_file_path("/a/../bad",full)==-1);assert(android_file_path("ux0:bad",full)==-1);
 unsigned char b[108];memset(b,0xaa,sizeof(b));assert(android_stat("/preferences/STSSound",b)==-1);assert(errno==2);assert(b[0]==0xaa);
 assert(!android_mkdir("/preferences",0700));FILE *f=android_fopen("/preferences/STSSound","wb");assert(f);int (*sv)(FILE*,char*,int,size_t)=(void*)android_log_lookup("setvbuf");assert(sv&&!sv(f,NULL,2,0));assert(fwrite("test",1,4,f)==4);assert(android_ftell(f)==4);assert(!android_fseek(f,1,SEEK_SET));assert(android_ftell(f)==1);fclose(f);
 f=android_fopen("/preferences/STSSound","rb");assert(f);char text[4];assert(fread(text,1,4,f)==4&&!memcmp(text,"test",4));fclose(f);
 assert(android_remove("../outside")==-1);
 assert(!android_stat("/preferences/STSSound",b));assert(word(b+48)==4);assert((word(b+16)&S_IFMT)==S_IFREG);assert(word(b+104)==0xaaaaaaaa);
 assert(!android_stat("/preferences",b));assert((word(b+16)&S_IFMT)==S_IFDIR);
 assert(!android_opendir("/missing")&&errno==2);
 assert(!android_opendir("../outside")&&errno==22);
 assert(!android_opendir("/preferences/STSSound")&&errno==20);
 assert(!android_mkdir("/preferences/empty",0700));
 void *dir=android_opendir("/preferences"),*other=android_opendir("/preferences");assert(dir&&other);
 unsigned seen=0;unsigned char *entry;
 while((entry=android_readdir(dir))){
  const char *name=(char*)entry+19;uint16_t length;memcpy(&length,entry+16,2);assert(length>=20+strlen(name)&&length<=280&&length%8==0);
  if(!strcmp(name,"STSSound")){assert(entry[18]==8);seen|=1;}
  else if(!strcmp(name,"empty")){assert(entry[18]==4);seen|=2;}
  else assert(!strcmp(name,".")||!strcmp(name,".."));
 }
 assert(errno==0&&seen==3);assert(android_readdir(other));assert(!android_closedir(dir));assert(!android_closedir(other));
 dir=android_opendir("/preferences/empty");assert(dir);
 while((entry=android_readdir(dir)))assert(!strcmp((char*)entry+19,".")||!strcmp((char*)entry+19,".."));
 assert(errno==0&&!android_closedir(dir));
 assert(!android_readdir(NULL)&&errno==9);assert(android_closedir(NULL)==-1&&errno==9);
 assert(!android_file_path("/preferences/empty",full));assert(!rmdir(full));
 assert(!android_file_path("/preferences/STSSound",full));assert(!android_remove("/preferences/STSSound"));assert(!android_file_path("/preferences",full));assert(!rmdir(full));assert(!rmdir(root));return 0;}
