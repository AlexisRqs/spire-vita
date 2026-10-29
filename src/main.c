#include <psp2/ctrl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/rng.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdatomic.h>
#include "debugScreen.h"
#include "elf_probe.h"
#include "android_tls.h"
#include "elf_image.h"
#include "compat_libc.h"
#include <malloc.h>
#include <vitaGL.h>
#include "bionic_sync.h"
#include "bionic_threads.h"
#include "bionic_local.h"
#include "bionic_cxa.h"
#include "android_assets.h"
#include "android_time.h"
#include "android_log.h"
#include "graphics_bridge.h"
#include "loading_screen.h"
#include "launcher_settings.h"
#include "launcher_background.h"
#include "description_strings.h"
#include "game_hooks.h"
#include "text_fast.h"
#include "text_submit.h"
#include "buffer_pointer.h"
#include "glyph_fast.h"
#include "card_profile.h"
#include "gameplay_profile.h"
#include "text_memo.h"
#include "map_cull.h"
#include "map_legend.h"
#include "lazy_character.h"
#include "load_profile.h"
#include "android_dlfcn.h"
#include "android_utf8.h"
#include "android_files.h"
#include "android_proc.h"
#include "preview_audio.h"
#include "preview_music.h"
#include "audio_hooks.h"
#include "preview_offline.h"
#include "image_view.h"
#include "direct_cache.h"
#include "direct_formats.h"
#include "keyboard.h"
#include "native_controller.h"
#include "android_platform.h"
#include "android_input.h"
#include "android_frame_loop.h"
#include "vita_frame.h"
#include "gc_probe.h"
#include "platform_probe.h"
#include "android_jump.h"
#include "bionic_modules.h"
/* kubridge public ABI; generated NID stubs are included in vendor/. */
int kuKernelMemProtect(void *address, SceSize size, SceUInt32 protection);
void kuKernelFlushCaches(const void *address, SceSize size);
int _newlib_heap_size_user=208*1024*1024;
static FILE *logfile;
static int graphics_live,launcher_owns_display;
static uint32_t engine_stack_guard;
static void note(const char *,...);
static void engine_abort(void) __attribute__((noreturn));
static void engine_abort(void){note("ENGINE ABORT / STACK CHECK FAILURE\n");sceKernelExitProcess(2);for(;;){}}

static pthread_mutex_t log_lock=PTHREAD_MUTEX_INITIALIZER;
static AndroidTls engine_tls;
static void *engine_tls_address(const AndroidTlsIndex *index) {
 return android_tls_address(&engine_tls,index);
}
static int resolve_import(const char *name,uint32_t *address,void *context) {
 if (!strcmp(name,"__tls_get_addr")) {
  *address=(uint32_t)(uintptr_t)engine_tls_address; return 1;
 }
 if(!strcmp(name,"__stack_chk_guard")){*address=(uint32_t)(uintptr_t)&engine_stack_guard;return 1;}
 uintptr_t found=0;
#define BIND(n,f) if (!strcmp(name,n)) found=(uintptr_t)f;
 BIND("abort",engine_abort)
 BIND("__stack_chk_fail",engine_abort)
 BIND("setjmp",android_setjmp)
 BIND("_setjmp",android__setjmp)
 BIND("sigsetjmp",android_sigsetjmp)
 BIND("longjmp",android_longjmp)
 BIND("_longjmp",android_longjmp)
 BIND("siglongjmp",android_siglongjmp)
 BIND("dl_iterate_phdr",bionic_dl_iterate_phdr)
 BIND("dl_unwind_find_exidx",bionic_dl_unwind_find_exidx)
#undef BIND
 if (!found) found=android_log_lookup(name);
 if (!found) found=compat_lookup(name);
 if (!found) found=bionic_sync_lookup(name);
 if (!found) found=bionic_threads_lookup(name);
 if (!found) found=bionic_local_lookup(name);
 if (!found) found=bionic_cxa_lookup(name);
 if (!found) found=android_assets_lookup(name);
 if (!found) found=android_time_lookup(name);
 if (!found) found=android_platform_lookup(name);
 if (!found) found=android_input_lookup(name);
 if (!found) found=android_frame_loop_lookup(name);
 if (!found) found=android_files_lookup(name);
 if (!found) found=android_proc_lookup(name);
 if (!found) found=android_utf8_lookup(name);
 if (!found) found=android_dlfcn_lookup(name);
 if (!found) found=graphics_lookup(name);
 if (found) { *address=(uint32_t)found; return 1; }
 if (context) fprintf((FILE *)context,"%s\n",name);
 return 0;
}
static uintptr_t lookup_linked(const char *name,void *context) {
 ElfImage *im=context;uint32_t slot,address;
 if (elf_image_relocation_target(im,name,22,&slot) && elf_image_relocation_target(im,name,21,&slot)) return 0;
 if (!elf_image_contains(im,slot,4,4)) return 0;
 memcpy(&address,im->base+slot,4); return address;
}
static void note(const char *fmt,...) {
 pthread_mutex_lock(&log_lock);
 char b[512];va_list a;va_start(a,fmt);vsnprintf(b,sizeof(b),fmt,a);va_end(a);
 if(!graphics_live&&!launcher_owns_display)psvDebugScreenPrintf("%s",b);
 if(logfile){fputs(b,logfile);fflush(logfile);}
 pthread_mutex_unlock(&log_lock);
}
void performance_clocks_start(void);
static void engine_log(int priority,const char *tag,const char *message){
 if(!strcmp(tag,"Android Application Output"))boot_display_message(message);
 /* Empty managed logger fragments need no disk write. Critical diagnostics
  * and stage boundaries still flush immediately; routine output is batched. */
 if(priority<5&&!message[strspn(message," \t\r\n")])return;
 static uint64_t last_flush;
 uint64_t now=sceKernelGetProcessTimeWide();
 pthread_mutex_lock(&log_lock);
 int flush=priority>=5||!strcmp(tag,"SceneProfile")||!strcmp(tag,"FrameProfile")||
           strstr(message,"load time:")||now-last_flush>=1000000;
 if(!graphics_live&&!launcher_owns_display)psvDebugScreenPrintf("Android[%d] %s: %s\n",priority,tag,message);
 if(logfile){fprintf(logfile,"Android[%d] %s: %s\n",priority,tag,message);if(flush){fflush(logfile);last_flush=now;}}
 pthread_mutex_unlock(&log_lock);
 if(strstr(message,"load time:")||strstr(message,"GC Warning")||strstr(message,"AL Init error")){
  struct mallinfo m=mallinfo();
  note("Memory: libc arena=%u used=%u free=%u heap_limit=%u\n",
   (unsigned)m.arena,(unsigned)m.uordblks,(unsigned)m.fordblks,(unsigned)_newlib_heap_size_user);
  unsigned views,avoided;image_view_stats(&views,&avoided);
  note("Image views: count=%u cumulative_copy_bytes_avoided=%u\n",views,avoided);

 }
}
static atomic_int probe_done;
static atomic_int game_running;
static void *probe_watchdog(void *unused){
 (void)unused;
 unsigned idle=0,last_frame=0;
 while(idle<(atomic_load(&game_running)?1800u:600u)){
  if(atomic_load(&probe_done))return NULL;
  sceKernelDelayThread(100000);
  unsigned frame=graphics_game_frames();
  if(frame!=last_frame){last_frame=frame;idle=0;}else idle++;
 }
 /* A blocked probe cannot produce more useful data; the last flushed stage
  * identifies the boundary. Exit instead of leaving an indefinite hang. */
 if(logfile){fputs("WATCHDOG: no progress within stage deadline (probe 60s, game 180s)\n",logfile);fflush(logfile);}
 sceKernelExitProcess(1);return NULL;
}
static void gc_stage(const char *name,uint32_t value){note("%s: %u\n",name,(unsigned)value);}
static int full_diagnostics=1;
static uint64_t launch_start,launch_mark;
static void boot_step(const char *name){
 uint64_t now=sceKernelGetProcessTimeWide();
 note("BootTiming: stage=%s elapsed_us=%llu since_start_us=%llu\n",name,(unsigned long long)(now-launch_mark),(unsigned long long)(now-launch_start));launch_mark=now;
}
static int flag_exists(const char *path){FILE *f=fopen(path,"rb");if(!f)return 0;fclose(f);return 1;}
static int diagnostic_mode(void){
 return !flag_exists("ux0:data/spireandroid/run-game.flag")||flag_exists("ux0:data/spireandroid/full-diagnostics.flag");
}
static int startup_gc(const GCProbeApi *api){
 if(full_diagnostics)return gc_probe_run(api,gc_stage);
 api->init();if(!*api->counter)return 3;
 api->allow_threads();return 0;
}
static void test_image(uint32_t span) {
 if (!span || span>64*1024*1024) { note("Image size unsupported\n"); return; }
 uint32_t rounded=(span+4095)&~4095u;
 SceUID block=sceKernelAllocMemBlock("SpireImage",SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,rounded,NULL);
 if (block<0) { note("Image block FAILED: 0x%08x\n",(unsigned)block); return; }
 uint32_t module_id=0;
 AudioRuntime *native_audio=NULL;AudioOutput *native_output=NULL;int native_audio_attached=0;
 int gc_started=0,watch_started=0;pthread_t watch;
 void *base=NULL;
 if (sceKernelGetMemBlockBase(block,&base)<0) { note("Image base FAILED\n"); goto finish; }
 ElfImage im; char error[128];
 note("Loading engine segments into dedicated block...\n");
 if (elf_image_load("ux0:data/spireandroid/libSpire_ANDROID.so",base,rounded,&im,error,sizeof(error))) {
  note("Image load FAILED: %s\n",error); goto finish;
 }
 boot_step("engine_read");
 if (bionic_module_add((uintptr_t)base,rounded,"libSpire_ANDROID.so",im.phdrs,(uint16_t)im.phnum,&module_id)) {
  note("Module registration FAILED\n"); goto finish;
 }
 note("Image mapped: %u segments at %p\n",(unsigned)im.count,base);
 if (android_tls_init(&engine_tls,im.tls_initial,im.tls_filesz,im.tls_size,im.tls_align)) {
  note("Engine TLS initialization FAILED\n"); goto finish;
 }
 FILE *imports=fopen("ux0:data/spireandroid/unresolved-imports.log","w");
 RelocReport report;
 int relocation=elf_image_relocate(&im,(uint32_t)(uintptr_t)base,resolve_import,imports,&report,error,sizeof(error));
 if (imports) fclose(imports);
 if (relocation) { note("Relocations FAILED: %s\n",error); goto finish; }
 note("Relocations applied=%u TLS=%u imports bound=%u missing=%u\n",
  (unsigned)report.applied,(unsigned)report.tls,(unsigned)report.bound,(unsigned)report.unresolved);
 boot_step("relocations");
 int index_result=elf_image_index_prepare(&im);
 note("Engine symbol index: %s\n",index_result<0?"invalid":index_result?"linear fallback":"ready");
 if(index_result<0)goto finish;
 boot_step("symbol_index");
 if(preview_audio_configure(&im)){note("Silent audio configuration FAILED\n");goto finish;}
 vita_keyboard_bind();
 if(keyboard_configure(&im)){note("Vita keyboard configuration FAILED\n");goto finish;}
 if(flag_exists("ux0:data/spireandroid/enable-card-profile.flag")&&card_profile_configure(&im)){note("Card CPU profiling configuration FAILED\n");goto finish;}
 gameplay_experiment_enable(flag_exists("ux0:data/spireandroid/enable-grid-replay.flag")?2:(flag_exists("ux0:data/spireandroid/enable-grid-experiment.flag")&&flag_exists("ux0:data/spireandroid/enable-gameplay-profile.flag")));
 if(load_profile_configure(&im)){note("Load profiling configuration FAILED\n");goto finish;}
 if(map_legend_configure(&im)){note("Map legend configuration FAILED\n");goto finish;}
 if(map_cull_configure(&im)){note("Map culling configuration FAILED\n");goto finish;}
 if(flag_exists("ux0:data/spireandroid/enable-description-strings.flag")&&description_strings_configure(&im)){note("Description string cache configuration FAILED\n");goto finish;}
 if(text_cache_capacity_enable(flag_exists("ux0:data/spireandroid/enable-large-text-cache.flag"))){note("Text cache capacity configuration FAILED\n");goto finish;}
 if(text_memo_configure(&im)){note("Text memo configuration FAILED\n");goto finish;}
 if(flag_exists("ux0:data/spireandroid/enable-buffer-pointer-test.flag")||flag_exists("ux0:data/spireandroid/enable-buffer-pointer.flag")){
  if(buffer_pointer_configure(&im)){note("Buffer pointer fast path unavailable; original retained\n");}
  else buffer_pointer_enable(flag_exists("ux0:data/spireandroid/enable-buffer-pointer.flag"));
 }
 if(!flag_exists("ux0:data/spireandroid/disable-text-submit.flag")&&text_submit_configure(&im)){note("Text submission fast path unavailable; original retained\n");}
 gameplay_sprite_profile_enable(flag_exists("ux0:data/spireandroid/enable-sprite-profile.flag"));
 /* Validate unmodified SpriteBatch entry before the transparent CPU profiler hooks it. */
 if((flag_exists("ux0:data/spireandroid/enable-gameplay-profile.flag")||flag_exists("ux0:data/spireandroid/enable-grid-replay.flag"))&&gameplay_profile_configure(&im)){note("Gameplay CPU profiling configuration FAILED\n");goto finish;}
 if(text_fast_configure(&im)){note("Text fast path configuration FAILED\n");goto finish;}
 text_vertices_enable(flag_exists("ux0:data/spireandroid/enable-text-vertices.flag"));
 text_integer_enable(flag_exists("ux0:data/spireandroid/enable-text-integer.flag"));
 text_rotation_enable(flag_exists("ux0:data/spireandroid/enable-text-rotation.flag"));
 text_profile_enable(flag_exists("ux0:data/spireandroid/enable-text-profile.flag"));
 if(flag_exists("ux0:data/spireandroid/enable-glyph-fast.flag")&&glyph_fast_configure(&im)){note("Glyph fast configuration FAILED\n");goto finish;}
 if(game_hooks_configure(&im)){note("Game instrumentation configuration FAILED\n");goto finish;}
 if(!flag_exists("ux0:data/spireandroid/disable-lazy-character.flag")){
  if(lazy_character_configure(&im)){note("Lazy character configuration FAILED\n");goto finish;}
  note("Lazy character selection enabled\n");
 }
 if(native_controller_configure(&im)){note("Native controller configuration FAILED\n");goto finish;}
 if(!flag_exists("ux0:data/spireandroid/disable-native-audio.flag")){
  native_audio=audio_runtime_create("ux0:data/spireandroid/assets",48000);
  if(native_audio)native_output=audio_output_start(native_audio);
 }
 if(native_audio&&native_output){
  if(audio_hooks_configure(&im,native_audio,native_output)){note("Native audio hook validation FAILED\n");goto finish;}
  native_audio_attached=1;
  note("Native audio: streaming IMA/Vorbis, 4 music + 28 effects, MAIN 48000Hz stereo, 512 frames, 4-block PCM queue\n");
 }else{
  audio_output_stop(native_output);native_output=NULL;
  audio_runtime_destroy(native_audio);native_audio=NULL;
  if(preview_music_configure(&im)){note("Silent music configuration FAILED\n");goto finish;}
  note("Native audio unavailable or disabled: silent fallback active\n");
 }
 if(preview_offline_configure(&im)){note("Offline service configuration FAILED\n");goto finish;}
 if(image_view_configure(&im)){note("KTX image view configuration FAILED\n");goto finish;}
 if(!flag_exists("ux0:data/spireandroid/disable-direct-cache.flag")){
  int formats=direct_formats_configure(&im);
  int cache=formats?-1:direct_cache_init("app0:direct-index.bin","ux0:data/spireandroid/texture-cache-033/data.bin","ux0:data/spireandroid/direct-cache-receipts.bin");
  if(!cache){android_asset_bind_override(direct_cache_read);
   if(!flag_exists("ux0:data/spireandroid/disable-loading-pack.flag"))direct_cache_loading_path("app0:loading-pack.bin");
  }
  note("Direct cache: %s (first-use source validation, subsequent stat receipt; payload checked every read)\n",cache?"unavailable; originals retained":"active");
 }
 note("KTX loading: GC-owned payload views enabled\n");
 note("Online services: unavailable; cloud sync task disabled (offline Vita preview)\n");
 note("Original OpenAL null backend retained; native audio output is separate\n");
 uint32_t tls_slot,tls_descriptor;
 if (elf_image_relocation_target(&im,"__tls_get_addr",22,&tls_slot) ||
     elf_image_relocation_target(&im,"GC_thread_curr",17,&tls_descriptor) ||
     !elf_image_contains(&im,tls_slot,4,4) || !elf_image_contains(&im,tls_descriptor,8,4)) {
  note("Engine TLS slots not found\n"); goto finish;
 }
 uint32_t resolver_address; AndroidTlsIndex index;
 memcpy(&resolver_address,im.base+tls_slot,4); memcpy(&index,im.base+tls_descriptor,8);
 if (resolver_address!=(uint32_t)(uintptr_t)engine_tls_address || index.module!=1 || index.offset!=16) {
  note("Engine TLS descriptor mismatch\n"); goto finish;
 }
 boot_step("adapters_and_tls_validation");
 if(full_diagnostics){
 void *(*linked_resolver)(const AndroidTlsIndex *)=(void *(*)(const AndroidTlsIndex *))(uintptr_t)resolver_address;
 int linked_result=android_tls_link_test(linked_resolver,&index);
 note("Engine GOT/TLS thread test: %s (code=%d)\n",linked_result?"FAILED":"OK",linked_result);
 if (linked_result) goto finish;
 int compat_result=compat_test(lookup_linked,&im);
 note("Linked libc + ARM float ABI test: %s (code=%d)\n",compat_result?"FAILED":"OK",compat_result);
 if (compat_result) goto finish;
 note("Testing Bionic mutex/condition bridge...\n");
 int sync_result=bionic_sync_test(lookup_linked,&im);
 note("Bionic sync ABI test: %s (code=%d)\n",sync_result?"FAILED":"OK",sync_result);
 if (sync_result) goto finish;
 }
 boot_step("abi_diagnostics");
 uint32_t value,size;
 if (elf_image_symbol(&im,"GC_get_version",&value,&size)) { note("Version symbol not found\n"); goto finish; }
 /* Exact leaf: movw r0,#0x500; movt r0,#7; bx lr. Refuse any other code.
  * No constructors, imports, relocations or GC initialization are needed. */
 const unsigned char expected[10]={0x40,0xf2,0x00,0x50,0xc0,0xf2,0x07,0x00,0x70,0x47};
 uint32_t code=value&~1u;
 if (!(value&1) || size!=sizeof(expected) || !elf_image_contains(&im,code,size,5) ||
     memcmp(im.base+code,expected,sizeof(expected))) {
  note("Version leaf differs: execution refused\n"); goto finish;
 }
 uint32_t page=code&~4095u, bytes=((code+size+4095)&~4095u)-page;
 int status=kuKernelMemProtect(im.base+page,bytes,0x40|0x10); /* read + execute */
 note("kubridge executable page: 0x%08x\n",(unsigned)status);
 if (status<0) goto finish;
 kuKernelFlushCaches(im.base+page,bytes);
 note("Calling verified GC_get_version leaf...\n");
 uint32_t (*version)(void)=(uint32_t (*)(void))(im.base+value);
 uint32_t actual=version();
 note("Native ARM call: %s, GC version=0x%08x\n",actual==0x70500?"OK":"FAILED",(unsigned)actual);
 if(actual!=0x70500)goto finish;
 /* Gate the executed runtime/GC path, including imported data objects. */
 const char *needed[]={"calloc","malloc","free","strlen","strcasestr","memcpy","memset",
  "pthread_mutex_init","pthread_mutex_lock","pthread_mutex_unlock","pthread_self",
  "pthread_getattr_np","pthread_attr_getstack","pthread_attr_destroy","setjmp",
  "dl_iterate_phdr","__tls_get_addr","stderr","__stack_chk_guard","__cxa_atexit",
  "ceilf","pthread_mutex_trylock","pthread_mutexattr_init","pthread_mutexattr_settype",
  "pthread_mutexattr_destroy","pthread_once","pthread_key_create","pthread_getspecific"};
 for(unsigned i=0;i<sizeof(needed)/sizeof(needed[0]);i++)if(!lookup_linked(needed[i],&im)){
  note("Runtime/GC dependency unavailable: %s\n",needed[i]);goto finish;
 }
 atomic_store(&probe_done,0);
 if(pthread_create(&watch,NULL,probe_watchdog,NULL)){note("Watchdog creation FAILED\n");goto finish;}
 watch_started=1;
 if(full_diagnostics){int platform=platform_probe(gc_stage);
 note("Grouped platform probe: %d\n",platform);if(platform)goto finish;}
 boot_step("execution_gates_and_platform_checks");
 GCProbeApi api={0};
 const char *exports[]={"GC_init","GC_allow_register_threads","GC_malloc","GC_add_roots",
  "GC_register_finalizer","GC_gcollect","GC_invoke_finalizers","GC_gc_no"};
 _Static_assert(sizeof(api)==8*sizeof(uintptr_t),"GC probe API layout");
 for(unsigned i=0;i<8;i++){
  uint32_t address,length;
  if(elf_image_symbol(&im,exports[i],&address,&length)||
     !elf_image_contains(&im,address&~1u,i==7?4:2,i==7?4:5)){
   note("GC export unavailable: %s\n",exports[i]);goto finish;
  }
  uintptr_t pointer=(uintptr_t)im.base+address;memcpy((char *)&api+i*sizeof(pointer),&pointer,sizeof(pointer));
 }
 for(unsigned i=0;i<im.count;i++)if(im.segments[i].flags&1){
  uint32_t first=im.segments[i].start&~4095u;
  uint32_t end=(im.segments[i].start+im.segments[i].size+4095u)&~4095u;
  if(kuKernelMemProtect(im.base+first,end-first,0x40|0x10)<0){note("GC executable segment protection FAILED\n");goto finish;}
  kuKernelFlushCaches(im.base+first,end-first);
 }
 gc_started=1;
 uint32_t init_offsets[129];unsigned init_count=0;
 if(elf_image_initializers(&im,(uint32_t)(uintptr_t)im.base,init_offsets,129,&init_count)){
  note("Initializer validation FAILED\n");goto finish;
 }
 note("Running %u validated ELF constructors\n",init_count);
 for(unsigned i=0;i<init_count;i++){
  note("Constructor %u begin\n",i);
  ((void (*)(void))(im.base+init_offsets[i]))();
  note("Constructor %u returned\n",i);
 }
 uint32_t runtime_offset,runtime_size;
 if(elf_image_symbol(&im,"_Z16BRUTE_InitializeiPPa",&runtime_offset,&runtime_size)||!elf_image_contains(&im,runtime_offset&~1u,2,5)){
  note("Runtime initializer unavailable\n");goto finish;
 }
 boot_step("elf_constructors");
 note("BRUTE_Initialize begin\n");
 char *runtime_argv[]={"Spire_ANDROID",NULL};
 void *managed_arguments=((void *(*)(int,char **))(im.base+runtime_offset))(1,runtime_argv);
 note("BRUTE_Initialize returned %p\n",managed_arguments);
 if(!managed_arguments)goto finish;
 boot_step("managed_runtime_init");
 int gc_result=startup_gc(&api);
 note("GC %s: %s (code=%d)\n",full_diagnostics?"full diagnostic":"initialization",gc_result?"FAILED":"OK",gc_result);
 if(gc_result)goto finish;
 boot_step("gc_startup");
 note("Initializing vitaGL memory pools and shader compiler\n");
 graphics_live=1;
 if(graphics_initialize()){note("vitaGL initialization FAILED\n");goto finish;}
 boot_step("graphics_init");
 launcher_background_stage(1,"Loading game resources");
 uint32_t app_symbol,app_size,context_offset,context_size;
 if(elf_image_symbol(&im,"_android_app",&app_symbol,&app_size)||!elf_image_contains(&im,app_symbol,4,6)||
    elf_image_symbol(&im,"nativePs4_createGlContext",&context_offset,&context_size)||!elf_image_contains(&im,context_offset&~1u,2,5)){
  note("Graphics entry points unavailable\n");goto finish;
 }
 /* This is a window-only bootstrap for context creation, not a full Android
  * activity/JNI/event loop. The full game entry is deliberately not run here. */
 static AndroidApp32 app;
 static AndroidActivity32 activity;
 static AndroidConfig config;
 static AndroidAssetManager assets;
 static uint32_t window;
 if(android_config_init(&config,"en","US")||android_asset_manager_init(&assets,"ux0:data/spireandroid/assets")){
  note("Android bootstrap configuration failed\n");goto finish;
 }
 activity.assets=(uint32_t)(uintptr_t)&assets;
 /* Virtual app root: ConsoleSave appends '/'. Supplying '/' here
  * produced '//preferences', rejected as an incomplete UNC path before IO. */
 activity.internal_path=(uint32_t)(uintptr_t)"";
 activity.external_path=activity.internal_path;activity.sdk=21;
 app.activity=(uint32_t)(uintptr_t)&activity;app.config=(uint32_t)(uintptr_t)&config;
 app.window=(uint32_t)(uintptr_t)&window;
 uint32_t size_setter,size_getter,symbol_size;
 if(elf_image_symbol(&im,"mobile_setDefaultScreenSize",&size_setter,&symbol_size)||
    elf_image_symbol(&im,"mobile_getDefaultScreenSize",&size_getter,&symbol_size)||
    !elf_image_contains(&im,size_setter&~1u,2,5)||!elf_image_contains(&im,size_getter&~1u,2,5)){
  note("Engine screen size API unavailable\n");goto finish;
 }
 ((void (*)(int,int))(im.base+size_setter))(960,544);
 int screen_width=0,screen_height=0;
 ((void (*)(int *,int *))(im.base+size_getter))(&screen_width,&screen_height);
 note("Engine screen size: %dx%d\n",screen_width,screen_height);
 if(screen_width!=960||screen_height!=544)goto finish;
 uint32_t app_address=(uint32_t)(uintptr_t)&app;memcpy(im.base+app_symbol,&app_address,4);
 note("Native graphics context creation begin\n");
 uintptr_t context=((uintptr_t (*)(void))(im.base+context_offset))();
 note("Native graphics context returned 0x%08x\n",(unsigned)context);
 if(!context)goto finish;
 boot_step("android_context_init");
 int gpu_result=0,shader_failures=0;
 if(full_diagnostics){
  gpu_result=graphics_probe("ux0:data/spireandroid/assets");
  note("Game shaders + ETC2 texture + GPU readback: %s (code=%d)\n",gpu_result?"FAILED":"OK",gpu_result);
  shader_failures=graphics_shader_probe("ux0:data/spireandroid/assets");
  note("All APK shader pairs: %s (failures=%d)\n",shader_failures?"FAILED":"OK",shader_failures);
 }else gpu_result=graphics_prepare_game();
 if(!gpu_result&&!shader_failures&&!launcher_background_active())note("Animated loading screen: %s\n",boot_display_start()?"unavailable":"active");
 boot_step("graphics_preflight");
 FILE *run_game=fopen("ux0:data/spireandroid/run-game.flag","rb");
 if(run_game){
  fclose(run_game);
  if(gpu_result||shader_failures){note("Game start rejected: GPU preflight failed\n");goto finish;}
  int missing=0;uint32_t bound;
#define STARTUP_IMPORT(name) if(!resolve_import(name,&bound,NULL)){note("Startup import missing: %s\n",name);missing++;}
#include "startup_imports.inc"
#undef STARTUP_IMPORT
  if(missing)goto finish;
  const char *names[]={"GameEngine","engine_handle_input","mobile_pressEscapeKey","_Z10BRUTE_MainPN6System6ArrayTIPNS_6StringELi1EEE"};
  uint32_t offsets[4],sizes[4];int valid=1;
  for(unsigned i=0;i<4;i++)if(elf_image_symbol(&im,names[i],&offsets[i],&sizes[i])||
    !elf_image_contains(&im,offsets[i]&~(i?1u:0u),i?2:56,i?5:6))valid=0;
  if(!valid){note("Game input/entry symbols unavailable\n");goto finish;}
  app.user=(uint32_t)(uintptr_t)(im.base+offsets[0]);
  app.input=(uint32_t)(uintptr_t)(im.base+offsets[1]);
  if(vita_frame_configure(&app,(EngineInput)(im.base+offsets[1]),(EngineBack)(im.base+offsets[2]))){note("Vita input initialization failed\n");goto finish;}
  elf_image_index_release();
  SceKernelFreeMemorySizeInfo memory={.size=sizeof(memory)};
  if(!sceKernelGetFreeMemorySize(&memory))note("Before game: free RAM=%u CDRAM=%u phycont=%u\n",memory.size_user,memory.size_cdram,memory.size_phycont);
  boot_step("game_entry_checks");
  graphics_observe_game(1);atomic_store(&game_running,1);
  note("BRUTE_Main begin; front touch and Cross/Circle enabled; Select+Start exits\n");
  ((void (*)(void *))(im.base+offsets[3]))(managed_arguments);
  note("BRUTE_Main returned after %u frames\n",graphics_game_frames());
  graphics_observe_game(0);atomic_store(&game_running,0);vita_frame_shutdown();
 }

finish:
 if(native_audio_attached)audio_hooks_disable();
 audio_output_stop(native_output);native_output=NULL;
 /* Bound hooks may still be referenced by engine threads: retain runtime until process exit. */
 if(!native_audio_attached)audio_runtime_destroy(native_audio);
 elf_image_index_release();
 boot_display_stop();
 if(watch_started){atomic_store(&probe_done,1);pthread_join(watch,NULL);}
 if(gc_started){note("GC image and TLS retained until process exit\n");return;}
 if (module_id && bionic_module_remove(module_id)) {
  note("Image still pinned: retaining block and TLS\n"); return;
 }
 android_tls_destroy(&engine_tls);
 note("Image block free: 0x%08x\n",(unsigned)sceKernelFreeMemBlock(block));
}
int main(void) {
 launch_start=launch_mark=sceKernelGetProcessTimeWide();
 if(sceKernelGetRandomNumber(&engine_stack_guard,sizeof(engine_stack_guard))<0||!engine_stack_guard)return 1;
 android_log_set_sink(engine_log);
 graphics_recording_configure(flag_exists("ux0:data/spireandroid/enable-grid-experiment.flag")||flag_exists("ux0:data/spireandroid/enable-grid-replay.flag"));
 android_dlfcn_configure(graphics_lookup);
 int isolated_grid_save=flag_exists("ux0:data/spireandroid/enable-grid-experiment.flag")&&flag_exists("ux0:data/spireandroid/use-grid-test-save.flag");
 android_files_configure(isolated_grid_save?"ux0:data/spireandroid/grid-test-files":"ux0:data/spireandroid/files");

 psvDebugScreenInit();mkdir("ux0:data/spireandroid",0777);
 mkdir("ux0:data/spireandroid/files",0777);
 /* Keep both launches for the cold/warm cache comparison. */
 remove("ux0:data/spireandroid/probe-previous.log");
 rename("ux0:data/spireandroid/probe.log","ux0:data/spireandroid/probe-previous.log");
 logfile=fopen("ux0:data/spireandroid/probe.log","w");
 note("GridTestSave: isolated=%d\n",isolated_grid_save);
 note("Spire Android Vita - diagnostic 0.72 (runtime + graphics + optional game)\n");
 launcher_owns_display=1;
 launcher_background_prepare();
 if(boot_display_start()){launcher_background_cancel();launcher_show();}
 /* Loading begins immediately; the launcher remains interactive on its worker. */
 launch_start=launch_mark=sceKernelGetProcessTimeWide();
 performance_clocks_start();
 full_diagnostics=diagnostic_mode();
 note("Launch mode: %s (full-diagnostics.flag enables full tests)\n",full_diagnostics?"diagnostic":"normal");
 note("Launch clock origin_us=%llu\n",(unsigned long long)launch_start);
 boot_step("launcher_setup");
 note("model=%d thread=%d log=%s\n",sceKernelGetModel(),sceKernelGetThreadId(),logfile?"OK":"FAILED");
 ElfProbe p;char error[128];
 if(elf_probe("ux0:data/spireandroid/libSpire_ANDROID.so",&p,error,sizeof(error))) {
  note("ELF validation FAILED: %s\n",error);
 } else {
  note("ELF ARM32 validated; LOAD segments=%u\n",(unsigned)p.load_segments);
  note("image span=%u bytes; TLS=%u bytes align=%u\n",(unsigned)p.image_span,(unsigned)p.tls_memsz,(unsigned)p.tls_align);
  note("TLS relocations: module=%u offset=%u\n",(unsigned)p.tls_module_relocs,(unsigned)p.tls_offset_relocs);
  note("other relocations=%u\n",(unsigned)p.other_relocs);
  if(full_diagnostics){
  note("Checking image-sized memory allocation...\n");
  if(p.image_span>64*1024*1024)note("Skipped: image exceeds diagnostic limit.\n");
  else {
   volatile unsigned char *mem=malloc(p.image_span);
   if(!mem)note("image allocation FAILED\n");
   else {for(uint32_t i=0;i<p.image_span;i+=4096)mem[i]=0x5a;
    note("image allocation + page writes OK\n");free((void*)mem);}
  }
 }
 }
 boot_step("elf_validation_and_optional_memory_test");
 int tls_result=0;
 if(full_diagnostics){
 note("Testing TLS: main + 4 concurrent threads, 2 rounds...\n");
 tls_result = android_tls_selftest();
 note("TLS isolation test: %s (code=%d)\n", tls_result ? "FAILED" : "OK", tls_result);
 }
 boot_step("tls_diagnostics");
 note("Next: relocate image and validate its TLS linkage.\n");
 if (!error[0] && !tls_result) test_image(p.image_span);
 note("\nDiagnostic complete. TRIANGLE exits.\n");
 note("Then reopen VitaShell FTP for log retrieval.\n");
 SceCtrlData pad;do {sceKernelDelayThread(100000);memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);} while(!(pad.buttons&SCE_CTRL_TRIANGLE));
 if(logfile) fclose(logfile);
 sceKernelExitProcess(0);
 return 0;
}
