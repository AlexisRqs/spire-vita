#ifndef PREVIEW_OFFLINE_H
#define PREVIEW_OFFLINE_H
#include "elf_image.h"
int preview_offline_configure(ElfImage *image);
int preview_offline_leaderboard_init(const char **name,const char **id);
int preview_offline_connected(void);
void preview_offline_gpg_init(void);
void preview_offline_begin_sync(void *object,int force);
void preview_offline_get_sync_stat(const char **identity,unsigned char *dirty);
void preview_offline_set_sync_stat(const char *identity,int dirty);
int preview_offline_cloud_time(int64_t *time,const char **identity,const char **metadata);
int preview_offline_cloud_data(void **data,int *size);
int preview_offline_cloud_save(const void *data,int size,int64_t time,const char *identity);
#endif
