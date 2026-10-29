#include "bionic_modules.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define LIMIT 16
#define PHDR_LIMIT 64
typedef struct Module {
 AndroidDlInfo info;
 uint32_t id,readers;
 char *name;
 AndroidPhdr *headers;
} Module;
static Module *modules[LIMIT];
static uint32_t serial;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
int bionic_module_add(uintptr_t base,size_t size,const char *name,
                      const AndroidPhdr *ph,uint16_t count,uint32_t *out) {
 if(!name||!ph||!out||!count||count>PHDR_LIMIT||!size||size>UINTPTR_MAX-base)return EINVAL;
 int loads=0;
 for(unsigned i=0;i<count;i++)if(ph[i].type==1) {
  if(ph[i].filesz>ph[i].memsz||ph[i].vaddr>size||ph[i].memsz>size-ph[i].vaddr)return EINVAL;
  loads++;
 }
 if(!loads)return EINVAL;
 unsigned exidx=0;
 for(unsigned i=0;i<count;i++)if(ph[i].type==0x70000001){
  if(++exidx>1||ph[i].filesz%8||ph[i].vaddr%4)return EINVAL;
  int contained=0;
  for(unsigned j=0;j<count;j++)if(ph[j].type==1&&(ph[j].flags&4)&&ph[i].vaddr>=ph[j].vaddr&&
    ph[i].vaddr-ph[j].vaddr<=ph[j].filesz&&ph[i].filesz<=ph[j].filesz-(ph[i].vaddr-ph[j].vaddr))contained=1;
  if(!contained)return EINVAL;
 }
 Module *m=calloc(1,sizeof(*m));if(!m)return ENOMEM;
 size_t n=strlen(name)+1;m->name=malloc(n);m->headers=malloc(count*sizeof(*ph));
 if(!m->name||!m->headers){free(m->name);free(m->headers);free(m);return ENOMEM;}
 memcpy(m->name,name,n);memcpy(m->headers,ph,count*sizeof(*ph));
 m->info=(AndroidDlInfo){base,m->name,m->headers,count};
 pthread_mutex_lock(&lock);unsigned slot;
 for(slot=0;slot<LIMIT;slot++)if(!modules[slot])break;
 int rc=0;
 if(slot==LIMIT||serial==UINT32_MAX)rc=ENOMEM;
 else {for(unsigned i=0;i<LIMIT;i++)if(modules[i]&&modules[i]->info.address==base)rc=EEXIST;}
 if(!rc){m->id=++serial;modules[slot]=m;*out=m->id;}
 pthread_mutex_unlock(&lock);
 if(rc){free(m->name);free(m->headers);free(m);}return rc;
}
int bionic_module_remove(uint32_t id) {
 pthread_mutex_lock(&lock);Module *m=NULL;unsigned slot;
 for(slot=0;slot<LIMIT;slot++)if(modules[slot]&&modules[slot]->id==id){m=modules[slot];break;}
 int rc=!m?ENOENT:m->readers?EBUSY:0;
 if(!rc)modules[slot]=NULL;
 pthread_mutex_unlock(&lock);
 if(!rc){free(m->name);free(m->headers);free(m);}return rc;
}
int bionic_dl_iterate_phdr(int (*callback)(AndroidDlInfo *,size_t,void *),void *data) {
 if(!callback)return -1;
 Module *snapshot[LIMIT];unsigned count=0;
 pthread_mutex_lock(&lock);
 for(unsigned i=0;i<LIMIT;i++)if(modules[i]){snapshot[count++]=modules[i];modules[i]->readers++;}
 pthread_mutex_unlock(&lock);
 int rc=0;
 for(unsigned i=0;i<count;i++) {
  AndroidDlInfo info=snapshot[i]->info;
  rc=callback(&info,sizeof(info),data);if(rc)break;
 }
 pthread_mutex_lock(&lock);
 for(unsigned i=0;i<count;i++)snapshot[i]->readers--;
 pthread_mutex_unlock(&lock);return rc;
}

uintptr_t bionic_dl_unwind_find_exidx(uintptr_t pc,int *count){
 if(!count)return 0;
 *count=0;uintptr_t result=0;pc&=~(uintptr_t)1;
 pthread_mutex_lock(&lock);
 for(unsigned i=0;i<LIMIT&&!result;i++)if(modules[i]){
  AndroidDlInfo *info=&modules[i]->info;int owns=0;
  if(pc<info->address)continue;
  uintptr_t offset=pc-info->address;
  for(unsigned j=0;j<info->count;j++){const AndroidPhdr *p=info->headers+j;
   if(p->type==1&&(p->flags&1)&&offset>=p->vaddr&&offset-p->vaddr<p->memsz)owns=1;
  }
  if(owns)for(unsigned j=0;j<info->count;j++){const AndroidPhdr *p=info->headers+j;
   if(p->type==0x70000001){result=info->address+p->vaddr;*count=p->filesz/8;break;}
  }
 }
 pthread_mutex_unlock(&lock);return result;
}
