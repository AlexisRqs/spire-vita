#include "graphics_guard.h"
#include <stdatomic.h>
#include <string.h>
typedef struct {uintptr_t target;char name[80];} Guard;
static Guard guards[GRAPHICS_GUARD_SLOTS];
static atomic_flag registry_lock=ATOMIC_FLAG_INIT;
static atomic_uint used,resolved;
static int enabled;
static atomic_int failed;
static int query(const char *name){
 static const char *const names[]={"glGetIntegerv","glGetFloatv","glGetBooleanv","glGetString","glIsEnabled","glGetUniformLocation","glGetAttribLocation","glGetProgramiv","glGetShaderiv","glGetShaderInfoLog","glGetProgramInfoLog","glCheckFramebufferStatus"};
 for(unsigned i=0;i<sizeof names/sizeof names[0];i++)if(!strcmp(name,names[i]))return 1;
 return 0;
}
int graphics_guard_configure(int value){if(atomic_load(&resolved))return -1;enabled=value!=0;return 0;}
int graphics_guard_ready(void){return enabled&&!atomic_load(&failed);}
uintptr_t graphics_guard_wrap(const char *name,uintptr_t target,int covered){
 if(!target||!name)return target;
 while(atomic_flag_test_and_set(&registry_lock)){}
 atomic_store(&resolved,1);
 if(!enabled||covered||query(name)){atomic_flag_clear(&registry_lock);return target;}
 unsigned count=atomic_load(&used);
 for(unsigned i=0;i<count;i++)if(guards[i].target==target&&!strcmp(guards[i].name,name)){uintptr_t result=graphics_guard_entries[i];atomic_flag_clear(&registry_lock);return result;}
 if(count==GRAPHICS_GUARD_SLOTS||strlen(name)>=sizeof guards[0].name||!graphics_guard_entries[count]){
  atomic_store(&failed,1);atomic_flag_clear(&registry_lock);return target;
 }
 unsigned index=count;guards[index].target=target;strcpy(guards[index].name,name);
 atomic_store_explicit(&used,count+1,memory_order_release);
 uintptr_t result=graphics_guard_entries[index];atomic_flag_clear(&registry_lock);return result;
}
uintptr_t ANDROID_ABI graphics_guard_hit(unsigned index){
 /* Entries are published once, before the pointer can be invoked. No reallocation
  * or registry writes touch published entries. Callback does not call the GPU. */
 if(index>=atomic_load_explicit(&used,memory_order_acquire)){atomic_store(&failed,1);return 0;}
 graphics_grid_unknown_call(guards[index].name);return guards[index].target;
}
