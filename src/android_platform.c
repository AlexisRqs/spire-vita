#include "android_platform.h"
#include <string.h>
int android_config_init(AndroidConfig *c,const char *language,const char *country){
 if(!c||!language||!country||strlen(language)!=2||strlen(country)!=2)return -1;
 for(unsigned i=0;i<2;i++)if(language[i]<'a'||language[i]>'z'||country[i]<'A'||country[i]>'Z')return -1;
 memcpy(c->language,language,2);memcpy(c->country,country,2);return 0;
}
void android_config_language(const AndroidConfig *c,char out[2]){if(out){if(c)memcpy(out,c->language,2);else memset(out,0,2);}}
void android_config_country(const AndroidConfig *c,char out[2]){if(out){if(c)memcpy(out,c->country,2);else memset(out,0,2);}}
uintptr_t android_platform_lookup(const char *name){
 if(!strcmp(name,"AConfiguration_getLanguage"))return (uintptr_t)android_config_language;
 if(!strcmp(name,"AConfiguration_getCountry"))return (uintptr_t)android_config_country;
 return 0;
}
