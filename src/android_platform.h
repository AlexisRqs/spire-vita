#ifndef ANDROID_PLATFORM_H
#define ANDROID_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
/* ARM32 prefixes from native_app_glue/native_activity. Remaining fields are
 * zeroed storage: no Android looper, JNI VM, or activity callbacks yet. */
typedef struct {
 uint32_t callbacks,vm,env,clazz,internal_path,external_path;
 int32_t sdk;
 uint32_t instance,assets,obb_path;
} AndroidActivity32;
typedef struct {
 uint32_t user,command,input,activity,config,saved_state,saved_size,looper,input_queue,window;
 uint32_t reserved[30];
} AndroidApp32;
_Static_assert(offsetof(AndroidActivity32,assets)==32,"ARM activity asset manager");
_Static_assert(offsetof(AndroidApp32,activity)==12,"ARM app activity");
_Static_assert(offsetof(AndroidApp32,config)==16,"ARM app config");
_Static_assert(offsetof(AndroidApp32,window)==36,"ARM app window");
typedef struct {char language[2],country[2];} AndroidConfig;
int android_config_init(AndroidConfig *config,const char *language,const char *country);
void android_config_language(const AndroidConfig *config,char out[2]);
void android_config_country(const AndroidConfig *config,char out[2]);
uintptr_t android_platform_lookup(const char *name);
#endif
