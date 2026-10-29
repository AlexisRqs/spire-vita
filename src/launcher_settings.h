#ifndef LAUNCHER_SETTINGS_H
#define LAUNCHER_SETTINGS_H
typedef struct {int effects;int diagnostics;int autoplay;} LauncherSettings;
/* effects: 0=game preference, 1=disabled, 2=enabled. */
extern LauncherSettings launcher_settings;
int launcher_settings_load(const char *path,LauncherSettings *out);
int launcher_settings_save(const char *path,const LauncherSettings *value);
void launcher_show(void);
#endif
