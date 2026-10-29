#include "launcher_settings.h"
#include <stdio.h>
#include <string.h>
LauncherSettings launcher_settings;
int launcher_settings_load(const char *path,LauncherSettings *out){
 *out=(LauncherSettings){0,0,0};FILE *f=fopen(path,"r");if(!f)return 1;
 int version,effects,diagnostics,autoplay=0;char extra;
 int n=fscanf(f,"spire_settings %d\neffects %d\ndiagnostics %d",&version,&effects,&diagnostics);
 int valid=n==3&&(version==1||version==2);
 if(valid&&version==2)valid=fscanf(f," autoplay %d",&autoplay)==1;
 if(fscanf(f," %c",&extra)==1)valid=0;
 int error=ferror(f);fclose(f);
 if(error||!valid||effects<0||effects>2||diagnostics<0||diagnostics>1||autoplay<0||autoplay>1)return -1;
 *out=(LauncherSettings){effects,diagnostics,autoplay};return 0;
}

int launcher_settings_save(const char *path,const LauncherSettings *v){
 if(v->effects<0||v->effects>2||v->diagnostics<0||v->diagnostics>1||v->autoplay<0||v->autoplay>1)return -1;
 char temp[256];if(snprintf(temp,sizeof(temp),"%s.tmp",path)>=(int)sizeof(temp))return -1;
 FILE *f=fopen(temp,"w");if(!f)return -1;
 int ok=fprintf(f,"spire_settings 2\neffects %d\ndiagnostics %d\nautoplay %d\n",v->effects,v->diagnostics,v->autoplay)>0;
 if(fflush(f))ok=0;
 if(fclose(f))ok=0;
 if(!ok){remove(temp);return -1;}
 if(rename(temp,path)){remove(temp);return -1;}
 return 0;
}
