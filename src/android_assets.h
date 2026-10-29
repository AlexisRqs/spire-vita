#ifndef ANDROID_ASSETS_H
#define ANDROID_ASSETS_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
typedef void *(*AndroidAssetOverride)(const char *,FILE *,size_t *);
void android_asset_bind_override(AndroidAssetOverride loader);
typedef struct {char root[1024];} AndroidAssetManager;
typedef struct AndroidAsset AndroidAsset;
int android_asset_manager_init(AndroidAssetManager *manager,const char *root);
AndroidAsset *android_asset_open(AndroidAssetManager *manager,const char *name,int mode);
int64_t android_asset_length(AndroidAsset *asset);
int android_asset_read(AndroidAsset *asset,void *buffer,size_t count);
void android_asset_close(AndroidAsset *asset);
uintptr_t android_assets_lookup(const char *name);
#endif
