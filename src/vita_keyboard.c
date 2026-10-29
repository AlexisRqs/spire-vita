#include "keyboard.h"
#include "android_log.h"
#include <psp2/ime_dialog.h>
#include <psp2/sysmodule.h>
#include <psp2/apputil.h>
static int initialized,app_ready;
static int begin(const uint16_t *title,const uint16_t *initial,uint16_t *output,unsigned capacity){
 int rc;const char *stage="AppUtil";
 if(!initialized){
  SceAppUtilInitParam app={0};SceAppUtilBootParam boot={0};
  if(!app_ready){rc=sceAppUtilInit(&app,&boot);if(rc<0)goto error;app_ready=1;}
  SceCommonDialogConfigParam config;sceCommonDialogConfigParamInit(&config);
  int language,button;stage="system language";
  rc=sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG,&language);if(rc<0)goto error;
  stage="system confirm button";
  rc=sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON,&button);if(rc<0)goto error;
  config.language=language;config.enterButtonAssign=button;
  stage="dialog config";rc=sceCommonDialogSetConfigParam(&config);if(rc<0)goto error;
  stage="IME module";
  if(sceSysmoduleIsLoaded(SCE_SYSMODULE_IME)!=SCE_SYSMODULE_LOADED){rc=sceSysmoduleLoadModule(SCE_SYSMODULE_IME);if(rc<0)goto error;}
  initialized=1;
 }
 SceImeDialogParam param;sceImeDialogParamInit(&param);
 param.type=SCE_IME_TYPE_DEFAULT;param.dialogMode=SCE_IME_DIALOG_DIALOG_MODE_WITH_CANCEL;
 param.title=title;param.initialText=(uint16_t*)initial;param.inputTextBuffer=output;param.maxTextLength=capacity-1;
 stage="IME dialog";rc=sceImeDialogInit(&param);if(rc<0)goto error;
 android_log_print(4,"Keyboard","Vita IME opened");return 0;
error:android_log_print(6,"Keyboard","Vita IME init failed at %s: 0x%x",stage,rc);return -1;
}
static int poll(void){
 int status=sceImeDialogGetStatus();if(status==SCE_COMMON_DIALOG_STATUS_RUNNING)return 0;
 if(status!=SCE_COMMON_DIALOG_STATUS_FINISHED)return -1;
 SceImeDialogResult result={0};int rc=sceImeDialogGetResult(&result);
 if(rc<0){android_log_print(6,"Keyboard","GetResult failed: 0x%x",rc);return -1;}
 int accepted=result.result==SCE_COMMON_DIALOG_RESULT_OK&&result.button==SCE_IME_DIALOG_BUTTON_ENTER;
 android_log_print(4,"Keyboard","Vita IME closed: accepted=%d result=%d button=%d",accepted,result.result,result.button);
 return accepted?1:2;
}
static void end(void){int rc=sceImeDialogTerm();if(rc<0)android_log_print(6,"Keyboard","Term failed: 0x%x",rc);}
void vita_keyboard_bind(void){keyboard_bind((KeyboardBackend){begin,poll,end});}
