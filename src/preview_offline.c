#include "preview_offline.h"
#include <string.h>
/* No Java/Google Play service on Vita. Report unavailable, never signed in.
 * Original native outputs point to static character buffers, not owned strings. */
int preview_offline_leaderboard_init(const char **name,const char **id){
 static const char empty[]="";
 if(name)*name=empty;
 if(id)*id=empty;
 return 0;
}
int preview_offline_connected(void){return 0;}
void preview_offline_gpg_init(void){}
/* APK 2.6.1 CloudSync layout, verified in BeginSync/IsSyncing/SyncTask.
 * A fresh offline instance never starts a task. Do not clear an existing task:
 * configuration is installed before managed initialization, not while syncing.
 * Completion progress is the same value as the original skip-sync branch. */
void preview_offline_begin_sync(void *object,int force){
 (void)force;
 if(!object)return;
 unsigned char *p=object;
 uint32_t task,complete=0x42c80000;
 memcpy(&task,p+12,4);
 if(task)return;
 memcpy(p+16,&complete,4);
 p[20]=0;
}
/* This API is local synchronization metadata, not a remote save listing.
 * Unknown identity + dirty: never claim a local save was synchronized.
 * Native bool* is ONE byte (STRB); the P/Invoke wrapper uses an int temporary. */
void preview_offline_get_sync_stat(const char **identity,unsigned char *dirty){
 if(identity)*identity=NULL;
 if(dirty)*dirty=1;
}
void preview_offline_set_sync_stat(const char *identity,int dirty){(void)identity;(void)dirty;}
int preview_offline_cloud_time(int64_t *time,const char **identity,const char **metadata){
 if(time)*time=0;
 if(identity)*identity=NULL;
 if(metadata)*metadata=NULL;
 return 0; /* Failure, never a successful empty cloud listing. */
}
int preview_offline_cloud_data(void **data,int *size){
 if(data)*data=NULL;
 if(size)*size=0;
 return 0;
}
int preview_offline_cloud_save(const void *data,int size,int64_t time,const char *identity){
 (void)data;(void)size;(void)time;(void)identity;return 0;
}
int preview_offline_configure(ElfImage *im){
 const char *names[]={"nativePs4_LeaderboadInit","nativePs4_getConnectionStatus","initGPG","GetCloudSyncStat","SetCloudSyncStat","GetCloudSyncTime","GetCloudData","SaveCloudData","_ZN5Spire5spire7console20CloudSync$_BeginSyncEPNS1_9CloudSyncEb"};
 uintptr_t targets[]={(uintptr_t)preview_offline_leaderboard_init,(uintptr_t)preview_offline_connected,(uintptr_t)preview_offline_gpg_init,(uintptr_t)preview_offline_get_sync_stat,(uintptr_t)preview_offline_set_sync_stat,(uintptr_t)preview_offline_cloud_time,(uintptr_t)preview_offline_cloud_data,(uintptr_t)preview_offline_cloud_save,(uintptr_t)preview_offline_begin_sync};
 enum { COUNT=sizeof(names)/sizeof(names[0]) };
 uint32_t slots[COUNT];
 if(!im||!im->relocated)return -1;
 /* Validate all slots before changing any of them. */
 for(unsigned i=0;i<COUNT;i++){
  uint32_t value,size,current;
  if(elf_image_symbol(im,names[i],&value,&size)||elf_image_relocation_target(im,names[i],22,&slots[i])||
    (slots[i]&3)||!elf_image_contains(im,slots[i],4,2)||!elf_image_contains(im,value&~1u,2,5))return -1;
  memcpy(&current,im->base+slots[i],4);
  if(current!=(uint32_t)(uintptr_t)(im->base+value))return -1;
 }
 for(unsigned i=0;i<COUNT;i++){uint32_t target=(uint32_t)targets[i];memcpy(im->base+slots[i],&target,4);}
 return 0;
}
