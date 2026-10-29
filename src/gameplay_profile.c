#include "gameplay_profile.h"
#include "compat_libc.h"
#include "scene_metrics.h"
#include "android_log.h"
#include <string.h>
#include <stdatomic.h>
#include <stdio.h>
#include "grid_experiment.h"
#include "grid_objects.h"
#include "grid_card_observer.h"
#include "graphics_bridge.h"
#include "text_submit.h"
static unsigned trace_frame;
static int boundary_pair_valid;
static GridExperiment experiment;
static int experiment_enabled,replay_default;
#define N 33
uint32_t gameplay_resumes[N];
static atomic_uint elapsed[N],calls[N],maximum[N];
static int enabled;
/* Frequent sprite hooks are diagnostic-only and require an explicit startup flag. */
static unsigned target_count=27;
int gameplay_sprite_profile_enable(int active){if(enabled)return -1;target_count=active?N:27;return 0;}
static const char *labels[N]={"animation","skeleton_json","atlas_data","camp_render","camp_update","grid_render","game_render","game_update","dungeon_update","room_render","hand_render","player_render","top_render","background_render","spine_render","top_name","top_hp","top_gold","top_dungeon","top_potions","top_relics","top_icons","top_controller","player_image","monster_render","spine_mesh","monster_intent","sprite_rotated","sprite_vertices","sprite_flush","mesh_render","vertex_bind","vertex_unbind"};
/* Inclusive times: children are part of parents. Never sum nested counters.
 * Inline entry hooks observe direct, PLT and virtual calls. */
static void record(unsigned i,uint64_t start){
 unsigned us=(unsigned)(scene_clock()-start);atomic_fetch_add(&elapsed[i],us);atomic_fetch_add(&calls[i],1);
 unsigned old=atomic_load(&maximum[i]);while(us>old&&!atomic_compare_exchange_weak(&maximum[i],&old,us)){}
 if(i<3)android_log_print(4,"GameplayLoad","name=%s begin_us=%llu inclusive_us=%u",labels[i],(unsigned long long)start,us);
}
extern void ANDROID_ABI gameplay_original_0(void *a,void *b,void *c,float d);
extern void * ANDROID_ABI gameplay_original_1(void *a,void *b);
extern void * ANDROID_ABI gameplay_original_2(void *a,void *b,void *c,int d);
extern void ANDROID_ABI gameplay_original_3(void *a,void *b);
extern void ANDROID_ABI gameplay_original_4(void *a);
extern void ANDROID_ABI gameplay_original_5(void *a,void *b);
static void ANDROID_ABI hook_0(void *a,void *b,void *c,float d){uint64_t t=scene_clock();gameplay_original_0(a,b,c,d);record(0,t);}
static void * ANDROID_ABI hook_1(void *a,void *b){uint64_t t=scene_clock();void *result=gameplay_original_1(a,b);record(1,t);return result;}
static void * ANDROID_ABI hook_2(void *a,void *b,void *c,int d){uint64_t t=scene_clock();void *result=gameplay_original_2(a,b,c,d);record(2,t);return result;}
static void ANDROID_ABI hook_3(void *a,void *b){uint64_t t=scene_clock();gameplay_original_3(a,b);record(3,t);}
static void ANDROID_ABI hook_4(void *a){uint64_t t=scene_clock();gameplay_original_4(a);record(4,t);}
static void ANDROID_ABI hook_5(void *a,void *b){
 if(experiment_enabled&&grid_experiment_skip(&experiment))return;
 int scope=experiment.mode!=6||grid_objects_plain(a,b);
 int boundary=experiment_enabled&&experiment.mode>=4&&scope;
 int isolated=!boundary||text_submit_flush_batch(b);
 if(experiment.ordinal==1)boundary_pair_valid=isolated;
 int report=trace_frame%120==0;
 int sample=scope&&experiment_enabled&&(experiment.mode==3||boundary)&&(report||experiment.mode==6);
 int trace_ok=sample&&isolated&&(!boundary||boundary_pair_valid);
 int observe=trace_ok?(report?1:2):0;
 graphics_grid_trace_begin(experiment.ordinal,observe);
 graphics_grid_stream_begin(experiment.ordinal,trace_frame,experiment.mode>=5?observe:0);
 grid_card_observer_begin(experiment.ordinal,trace_frame,experiment.mode>=5?observe:0);
 grid_objects_begin(a,b,experiment.ordinal,trace_frame,experiment.mode>=5?observe:0);
 uint64_t t=scene_clock();int reused=0;
 if(experiment.mode==6&&experiment.ordinal==2&&trace_ok){
  GridBatchPatch patch;int objects=grid_objects_prepare_replay(a,b,trace_frame,&patch);
  int cards=grid_card_observer_can_reuse(trace_frame);int gpu=graphics_grid_replay_ready(trace_frame);
  if(objects&&cards&&gpu&&graphics_grid_replay(trace_frame)){grid_batch_apply(b,&patch);reused=1;experiment.skipped++;}
  if(report)android_log_print(4,"GridReplay","frame=%u applied=%d objects=%d cards=%d gpu=%d",trace_frame,reused,objects,cards,gpu);
 }
 if(!reused&&experiment.mode==6&&experiment.ordinal==2){graphics_grid_trace_begin(0,0);graphics_grid_stream_begin(0,trace_frame,0);}
 if(!reused)gameplay_original_5(a,b);record(5,t);
 if(boundary){int end_ok=text_submit_flush_batch(b);if(!end_ok){boundary_pair_valid=0;graphics_grid_trace_begin(0,0);graphics_grid_stream_begin(0,0,0);}if(sample&&report)android_log_print(4,"GridBoundary","pass=%u begin=%d end=%d",experiment.ordinal,isolated,end_ok);}
 grid_card_observer_end();grid_objects_end();graphics_grid_trace_end();graphics_grid_stream_end();
}
extern void ANDROID_ABI gameplay_original_6(void *a);
static void ANDROID_ABI hook_6(void *a){uint64_t t=scene_clock();gameplay_original_6(a);record(6,t);}
extern void ANDROID_ABI gameplay_original_7(void *a);
static void ANDROID_ABI hook_7(void *a){uint64_t t=scene_clock();gameplay_original_7(a);record(7,t);}
extern void ANDROID_ABI gameplay_original_8(void *a);
static void ANDROID_ABI hook_8(void *a){uint64_t t=scene_clock();gameplay_original_8(a);record(8,t);}
extern void ANDROID_ABI gameplay_original_9(void *a,void *b);
static void ANDROID_ABI hook_9(void *a,void *b){uint64_t t=scene_clock();gameplay_original_9(a,b);record(9,t);}
extern void ANDROID_ABI gameplay_original_10(void *a,void *b);
static void ANDROID_ABI hook_10(void *a,void *b){uint64_t t=scene_clock();gameplay_original_10(a,b);record(10,t);}
extern void ANDROID_ABI gameplay_original_11(void *a,void *b);
static void ANDROID_ABI hook_11(void *a,void *b){uint64_t t=scene_clock();gameplay_original_11(a,b);record(11,t);}
extern void ANDROID_ABI gameplay_original_12(void *a,void *b);
static void ANDROID_ABI hook_12(void *a,void *b){uint64_t t=scene_clock();gameplay_original_12(a,b);record(12,t);}
extern void ANDROID_ABI gameplay_original_13(void *a,void *b);
static void ANDROID_ABI hook_13(void *a,void *b){uint64_t t=scene_clock();gameplay_original_13(a,b);record(13,t);}
extern void ANDROID_ABI gameplay_original_14(void *a,void *b,void *c);
static void ANDROID_ABI hook_14(void *a,void *b,void *c){uint64_t t=scene_clock();gameplay_original_14(a,b,c);record(14,t);}
extern void ANDROID_ABI gameplay_original_15(void *a,void *b);
static void ANDROID_ABI hook_15(void *a,void *b){uint64_t t=scene_clock();gameplay_original_15(a,b);record(15,t);}
extern void ANDROID_ABI gameplay_original_16(void *a,void *b);
static void ANDROID_ABI hook_16(void *a,void *b){uint64_t t=scene_clock();gameplay_original_16(a,b);record(16,t);}
extern void ANDROID_ABI gameplay_original_17(void *a,void *b);
static void ANDROID_ABI hook_17(void *a,void *b){uint64_t t=scene_clock();gameplay_original_17(a,b);record(17,t);}
extern void ANDROID_ABI gameplay_original_18(void *a,void *b);
static void ANDROID_ABI hook_18(void *a,void *b){uint64_t t=scene_clock();gameplay_original_18(a,b);record(18,t);}
extern void ANDROID_ABI gameplay_original_19(void *a,void *b);
static void ANDROID_ABI hook_19(void *a,void *b){uint64_t t=scene_clock();gameplay_original_19(a,b);record(19,t);}
extern void ANDROID_ABI gameplay_original_20(void *a,void *b);
static void ANDROID_ABI hook_20(void *a,void *b){uint64_t t=scene_clock();gameplay_original_20(a,b);record(20,t);}
extern void ANDROID_ABI gameplay_original_21(void *a,void *b);
static void ANDROID_ABI hook_21(void *a,void *b){uint64_t t=scene_clock();gameplay_original_21(a,b);record(21,t);}
extern void ANDROID_ABI gameplay_original_22(void *a,void *b);
static void ANDROID_ABI hook_22(void *a,void *b){uint64_t t=scene_clock();gameplay_original_22(a,b);record(22,t);}
extern void ANDROID_ABI gameplay_original_23(void *a,void *b);
static void ANDROID_ABI hook_23(void *a,void *b){uint64_t t=scene_clock();gameplay_original_23(a,b);record(23,t);}
extern void ANDROID_ABI gameplay_original_24(void *a,void *b);
static void ANDROID_ABI hook_24(void *a,void *b){uint64_t t=scene_clock();gameplay_original_24(a,b);record(24,t);}
extern void ANDROID_ABI gameplay_original_25(void *a,void *b,void *c);
static void ANDROID_ABI hook_25(void *a,void *b,void *c){uint64_t t=scene_clock();gameplay_original_25(a,b,c);record(25,t);}
extern void ANDROID_ABI gameplay_original_26(void *a,void *b);
static void ANDROID_ABI hook_26(void *a,void *b){uint64_t t=scene_clock();gameplay_original_26(a,b);record(26,t);}
extern void ANDROID_ABI gameplay_original_27(void *a,void *b,float c,float d,float e,float f,float g,float h,float j,float k,float l);
static void ANDROID_ABI hook_27(void *a,void *b,float c,float d,float e,float f,float g,float h,float j,float k,float l){uint64_t t=scene_clock();gameplay_original_27(a,b,c,d,e,f,g,h,j,k,l);record(27,t);}
extern void ANDROID_ABI gameplay_original_28(void *a,void *b,void *c,int d,int e);
static void ANDROID_ABI hook_28(void *a,void *b,void *c,int d,int e){uint64_t t=scene_clock();gameplay_original_28(a,b,c,d,e);record(28,t);}
extern void ANDROID_ABI gameplay_original_29(void *a);
static void ANDROID_ABI hook_29(void *a){uint64_t t=scene_clock();gameplay_original_29(a);record(29,t);}
extern void ANDROID_ABI gameplay_original_30(void *a,void *b,int c,int d,int e,int f);
static void ANDROID_ABI hook_30(void *a,void *b,int c,int d,int e,int f){uint64_t t=scene_clock();gameplay_original_30(a,b,c,d,e,f);record(30,t);}
extern void ANDROID_ABI gameplay_original_31(void *a,void *b,void *c);
static void ANDROID_ABI hook_31(void *a,void *b,void *c){uint64_t t=scene_clock();gameplay_original_31(a,b,c);record(31,t);}
extern void ANDROID_ABI gameplay_original_32(void *a,void *b,void *c);
static void ANDROID_ABI hook_32(void *a,void *b,void *c){uint64_t t=scene_clock();gameplay_original_32(a,b,c);record(32,t);}
static const struct {const char *name,*label;uint32_t value,size,length;unsigned char signature[10];void *hook;int load;} targets[N]={
{"_ZN3com8megacrit9cardcrawl4core36AbstractCreature$_loadAnimation_VoidEPNS2_16AbstractCreatureEPN6System6StringES7_f","animation",0x1ef3e45,852,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x4,0x8b},(void*)hook_0,1},
{"_ZN3com16esotericsoftware5spine43SkeletonJson$_readSkeletonData_SkeletonDataEPNS1_12SkeletonJsonEPNS_8badlogic3gdx5files10FileHandleE","skeleton_json",0x1e086c5,6696,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x2,0x8b},(void*)hook_1,1},
{"_ZN3com8badlogic3gdx8graphics3g2d36TextureAtlas$TextureAtlasData$_$CtorEPNS3_29TextureAtlas$TextureAtlasDataEPNS1_5files10FileHandleES8_b","atlas_data",0x1db39b9,3196,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x6,0x8b},(void*)hook_2,1},
{"_ZN3com8megacrit9cardcrawl5rooms23CampfireUI$_render_VoidEPNS2_10CampfireUIEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","camp_render",0x20d9bc5,928,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x8,0x8b},(void*)hook_3,0},
{"_ZN3com8megacrit9cardcrawl5rooms23CampfireUI$_update_VoidEPNS2_10CampfireUIE","camp_update",0x20d87f9,540,8,{0x2d,0xe9,0xf0,0x41,0x2d,0xed,0x2,0x8b},(void*)hook_4,0},
{"_ZN3com8megacrit9cardcrawl7screens6select33GridCardSelectScreen$_render_VoidEPNS3_20GridCardSelectScreenEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","grid_render",0x2198705,3144,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x6,0x8b},(void*)hook_5,0},
{"_ZN3com8megacrit9cardcrawl4core26CardCrawlGame$_render_VoidEPNS2_13CardCrawlGameE","game_render",0x1efaab1,2112,8,{0x2d,0xe9,0xf0,0x41,0x82,0xb0,0x80,0x46},(void*)hook_6,0},
{"_ZN3com8megacrit9cardcrawl4core26CardCrawlGame$_update_VoidEPNS2_13CardCrawlGameE","game_update",0x1efdf49,3928,8,{0x2d,0xe9,0xf0,0x4f,0x8f,0xb0,0x82,0x46},(void*)hook_7,0},
{"_ZN3com8megacrit9cardcrawl8dungeons28AbstractDungeon$_update_VoidEPNS2_15AbstractDungeonE","dungeon_update",0x1f3007d,4320,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x2,0x8b},(void*)hook_8,0},
{"_ZN3com8megacrit9cardcrawl5rooms25AbstractRoom$_render_VoidEPNS2_12AbstractRoomEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","room_render",0x20d5015,1168,8,{0x2d,0xe9,0xf0,0x47,0x88,0xb0,0x80,0x46},(void*)hook_9,0},
{"_ZN3com8megacrit9cardcrawl10characters31AbstractPlayer$_renderHand_VoidEPNS2_14AbstractPlayerEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","hand_render",0x1edf291,1712,8,{0x2d,0xe9,0xf0,0x4f,0x85,0xb0,0x5,0x46},(void*)hook_10,0},
{"_ZN3com8megacrit9cardcrawl10characters27AbstractPlayer$_render_VoidEPNS2_14AbstractPlayerEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","player_render",0x1ede2b1,1356,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0xc,0x8b},(void*)hook_11,0},
{"_ZN3com8megacrit9cardcrawl2ui6panels21TopPanel$_render_VoidEPNS3_8TopPanelEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","top_render",0x2203edd,1636,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x2,0x8b},(void*)hook_12,0},
{"_ZN3com8megacrit9cardcrawl6scenes39TheBottomScene$_renderCombatRoomBg_VoidEPNS2_14TheBottomSceneEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","background_render",0x20e7d65,1044,8,{0x2d,0xe9,0xf0,0x41,0x8a,0xb0,0x5,0x46},(void*)hook_13,0},
{"_ZN3com16esotericsoftware5spine29SkeletonRenderer$1$_draw_VoidIPNS_8badlogic3gdx8graphics3g2d18PolygonSpriteBatchEEEvPNS1_18SkeletonRenderer$1IT_EESA_PNS1_8SkeletonE","spine_render",0x29baa49,1696,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x8,0x8b},(void*)hook_14,0},
{"_ZN3com8megacrit9cardcrawl2ui6panels20TopPanel$_renderNameEPNS3_8TopPanelEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","top_name",0x2204541,1120,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x6,0x8b},(void*)hook_15,0},
{"_ZN3com8megacrit9cardcrawl2ui6panels18TopPanel$_renderHPEPNS3_8TopPanelEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","top_hp",0x22049a1,1004,8,{0x2d,0xe9,0xf0,0x4f,0x8d,0xb0,0x80,0x46},(void*)hook_16,0},
{"_ZN3com8megacrit9cardcrawl2ui6panels20TopPanel$_renderGoldEPNS3_8TopPanelEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","top_gold",0x2204d8d,1076,8,{0x2d,0xe9,0xf0,0x43,0x8d,0xb0,0x80,0x46},(void*)hook_17,0},
{"_ZN3com8megacrit9cardcrawl2ui6panels27TopPanel$_renderDungeonInfoEPNS3_8TopPanelEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","top_dungeon",0x22051c1,760,10,{0x2d,0xe9,0xf0,0x4f,0x89,0xb0,0x5,0x90,0x89,0x46},(void*)hook_18,0},
{"_ZN3com8megacrit9cardcrawl2ui6panels23TopPanel$_renderPotionsEPNS3_8TopPanelEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","top_potions",0x2206a21,796,8,{0x2d,0xe9,0xf0,0x43,0x87,0xb0,0x5,0x46},(void*)hook_19,0},
{"_ZN3com8megacrit9cardcrawl2ui6panels22TopPanel$_renderRelicsEPNS3_8TopPanelEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","top_relics",0x2206629,1016,8,{0x2d,0xe9,0xf0,0x4f,0x8f,0xb0,0x7,0x46},(void*)hook_20,0},
{"_ZN3com8megacrit9cardcrawl2ui6panels29TopPanel$_renderTopRightIconsEPNS3_8TopPanelEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","top_icons",0x2205b11,2840,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x4,0x8b},(void*)hook_21,0},
{"_ZN3com8megacrit9cardcrawl2ui6panels28TopPanel$_renderControllerUiEPNS3_8TopPanelEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","top_controller",0x2206d3d,2428,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x6,0x8b},(void*)hook_22,0},
{"_ZN3com8megacrit9cardcrawl10characters38AbstractPlayer$_renderPlayerImage_VoidEPNS2_14AbstractPlayerEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","player_image",0x1ede941,1408,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0xc,0x8b},(void*)hook_23,0},
{"_ZN3com8megacrit9cardcrawl8monsters28AbstractMonster$_render_VoidEPNS2_15AbstractMonsterEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","monster_render",0x1ffd3dd,2464,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0xe,0x8b},(void*)hook_24,0},
{"_ZN3com16esotericsoftware5spine31SkeletonMeshRenderer$_draw_VoidEPNS1_20SkeletonMeshRendererEPNS_8badlogic3gdx8graphics3g2d18PolygonSpriteBatchEPNS1_8SkeletonE","spine_mesh",0x1e0dd4d,1836,10,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x8,0x8b},(void*)hook_25,0},
{"_ZN3com8megacrit9cardcrawl8monsters29AbstractMonster$_renderIntentEPNS2_15AbstractMonsterEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE","monster_intent",0x1ffdeb9,1276,8,{0x2d,0xe9,0xf0,0x41,0x2d,0xed,0x2,0x8b},(void*)hook_26,0},
{"_ZN3com8badlogic3gdx8graphics3g2d22SpriteBatch$_draw_VoidEPNS3_11SpriteBatchEPNS3_13TextureRegionEfffffffff","sprite_rotated",0x1db121d,1564,10,{0x2d,0xe9,0xf0,0x43,0x81,0xb0,0x2d,0xed,0xe,0x8b},(void*)hook_27,0},
{"_ZN3com8badlogic3gdx8graphics3g2d22SpriteBatch$_draw_VoidEPNS3_11SpriteBatchEPNS2_7TextureEPN6System6ArrayTIfLi1EEEii","sprite_vertices",0x1db0bb5,340,8,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x82,0x46},(void*)hook_28,0},
{"_ZN3com8badlogic3gdx8graphics3g2d23SpriteBatch$_flush_VoidEPNS3_11SpriteBatchE","sprite_flush",0x1db1839,720,8,{0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x4,0x46},(void*)hook_29,0},
{"_ZN3com8badlogic3gdx8graphics17Mesh$_render_VoidEPNS2_4MeshEPNS2_7glutils13ShaderProgramEiiib","mesh_render",0x1dc6195,1180,8,{0x2d,0xe9,0xf0,0x4f,0x85,0xb0,0x80,0x46},(void*)hook_30,0},
{"_ZN3com8badlogic3gdx8graphics7glutils22VertexArray$_bind_VoidEPNS3_11VertexArrayEPNS3_13ShaderProgramEPN6System6ArrayTIiLi1EEE","vertex_bind",0x1dc23b5,1172,8,{0x2d,0xe9,0xf0,0x4f,0x85,0xb0,0x82,0x46},(void*)hook_31,0},
{"_ZN3com8badlogic3gdx8graphics7glutils24VertexArray$_unbind_VoidEPNS3_11VertexArrayEPNS3_13ShaderProgramEPN6System6ArrayTIiLi1EEE","vertex_unbind",0x1dc285d,424,8,{0x2d,0xe9,0xf0,0x43,0x81,0xb0,0x80,0x46},(void*)hook_32,0}
};

void gameplay_profile_report(void){
 if(!enabled)return;
 if(experiment_enabled){android_log_print(4,"GridExperiment","mode=%u serial=%u seen=%u skipped=%u bad_frames=%u",experiment.mode,experiment.serial,experiment.seen,experiment.skipped,experiment.bad);experiment.seen=experiment.skipped=experiment.bad=0;}
 for(unsigned i=0;i<target_count;i++){
  unsigned us=atomic_exchange(&elapsed[i],0),count=atomic_exchange(&calls[i],0),max=atomic_exchange(&maximum[i],0);
  android_log_print(4,"GameplayCPU","name=%s calls=%u inclusive_us=%u max_us=%u",labels[i],count,us,max);
 }
}
int gameplay_profile_configure(ElfImage *im){
 if(!im||!im->relocated||enabled)return -1;
 /* Validate ALL sites before writing any jump. Exact APK version only. */
 for(unsigned i=0;i<target_count;i++){
  uint32_t v,size;
  if(elf_image_symbol(im,targets[i].name,&v,&size)||v!=targets[i].value||size!=targets[i].size||
     (v&3)!=1||!elf_image_contains(im,v&~1u,targets[i].length,5)||
     memcmp(im->base+(v&~1u),targets[i].signature,targets[i].length))return -1;
 }
 for(unsigned i=0;i<target_count;i++){
  uint32_t at=targets[i].value&~1u,target=(uint32_t)(uintptr_t)targets[i].hook;
  gameplay_resumes[i]=(uint32_t)(uintptr_t)(im->base+at+targets[i].length)|1;
  const unsigned char jump[]={0xdf,0xf8,0,0xf0};
  memcpy(im->base+at,jump,4);memcpy(im->base+at+4,&target,4);
 }
 if(!grid_objects_configure(im))grid_card_observer_configure(im);
 enabled=1;return 0;
}

void gameplay_experiment_enable(int enable){experiment_enabled=enable!=0;replay_default=enable==2;grid_experiment_set(&experiment,replay_default?6:0,0);}
void gameplay_experiment_present(unsigned frame){
 trace_frame=frame;
 if(!experiment_enabled)return;
 if(replay_default&&experiment.mode==6)experiment.frames=0;
 unsigned previous_mode=experiment.mode;grid_experiment_frame(&experiment);
 if(previous_mode&&!experiment.mode)android_log_print(4,"GridExperimentChange","mode=0 serial=%u frame=%u watchdog=1",experiment.serial,frame);
 if(frame%120)return;
 const char *path="ux0:data/spireandroid/grid-experiment.txt";
 FILE *f=fopen(path,"r");if(!f)return;
 unsigned mode,serial;char extra;int n=fscanf(f,"grid_test %u %u %c",&mode,&serial,&extra);int failed=ferror(f);fclose(f);
 if(remove(path))return;
 if(failed||n!=2||mode>6){grid_experiment_set(&experiment,0,0);return;}
 grid_experiment_set(&experiment,mode,serial);
 android_log_print(4,"GridExperimentChange","mode=%u serial=%u frame=%u watchdog=0",mode,serial,frame);
}
