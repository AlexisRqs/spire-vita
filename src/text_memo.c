#include "text_memo.h"
#include "compat_libc.h"
#include "android_log.h"
#include <stdint.h>
#include <string.h>
/* Dedicated layouts only; never retain FontHelper.layout or a pooled layout.
 * Four-way set associative cache, bounded text length and object count. */
#define MEMO_CHARS 48
#ifdef SPIRE_EXPERIMENTAL_LARGE_TEXT_CACHE
#define MEMO_SLOTS 512
#define VERTEX_STORAGE_SLOTS 1024
#else
#define MEMO_SLOTS 128
#define VERTEX_STORAGE_SLOTS 256
#endif
static unsigned memo_active_slots=128,vertex_active_slots=256;
static int memo_started;
/* Startup-only switch: never reinterpret populated hash tables. */
int text_cache_capacity_enable(int enabled){
 if(memo_started)return -1;
 memo_active_slots=enabled?MEMO_SLOTS:128;vertex_active_slots=enabled?VERTEX_STORAGE_SLOTS:256;return 0;
}
static void *(ANDROID_ABI *memo_create)(void*);
static void (ANDROID_ABI *memo_set)(void*,void*,void*);
static void *(ANDROID_ABI *memo_glyph)(void*,unsigned);
static int (ANDROID_ABI *memo_kerning)(void*,unsigned);
static void (ANDROID_ABI *memo_roots)(void*,void*);
static void (ANDROID_ABI *memo_pack)(void*,int);
static uintptr_t memo_base_runs,memo_ft_runs,memo_ft_glyph,memo_clear,memo_draw;
static uint32_t *memo_gc_stop;
static unsigned char *memo_data_init;
static int memo_rooted,memo_disabled;
static unsigned memo_hits,memo_misses,memo_bypass,memo_invalid,memo_evict,memo_checks,memo_mismatch;
static uint32_t memo_clock;
typedef struct {
 void *font,*data,*layout;
 uint32_t age,valid,length;
 unsigned char data_state[96],color[16];
 uint16_t text[MEMO_CHARS];
 void *glyph[MEMO_CHARS];
 unsigned char glyph_state[MEMO_CHARS][56];
 int kerning[MEMO_CHARS];
 unsigned vertex_generation;
} MemoEntry;
static MemoEntry memo_entries[MEMO_SLOTS+1];
static MemoEntry *memo_current;
static int vertices_enabled,vertices_disabled;
static unsigned vertex_hits,vertex_misses,vertex_bypass,vertex_checks,vertex_mismatch;
static void *mp(void *p,unsigned n){uint32_t v;memcpy(&v,(char*)p+n,4);return (void*)(uintptr_t)v;}
static uint32_t mw(void *p,unsigned n){uint32_t v;memcpy(&v,(char*)p+n,4);return v;}
static uintptr_t mm(void *p,unsigned n){
 if(!p)return 0;
 void *type=mp(p,0);if(!type)return 0;
 void *vt=mp(type,4);if(!vt||*((unsigned char*)vt+n+4))return 0;
 return (uintptr_t)mp(vt,n);
}
static int glyphs_match(MemoEntry *e,void *data,const uint16_t *chars,unsigned n,int save){
 void *previous=NULL;
 for(unsigned i=0;i<n;i++){
  void *g=memo_glyph(data,chars[i]);
  if(!g||mm(g,32)!=(uintptr_t)memo_kerning)return 0;
  int k=previous?memo_kerning(previous,chars[i]):0;
  if(save){e->glyph[i]=g;memcpy(e->glyph_state[i],(char*)g+8,56);e->kerning[i]=k;}
  else if(e->glyph[i]!=g||memcmp(e->glyph_state[i],(char*)g+8,56)||e->kerning[i]!=k)return 0;
  previous=g;
 }
 return 1;
}
static void *array_data(void *a,unsigned count){
 if(!a)return NULL;
 void *items=mp(a,8);if(!items)return NULL;
 void *shape=mp(items,20);if(!shape||mw(shape,0)<count)return NULL;
 return mp(items,8);
}
static int same_array(void *a,void *b){
 if(!a||!b)return a==b;
 unsigned n=mw(a,12);if(n!=mw(b,12)||n>MEMO_CHARS+1)return 0;
 if(!n)return 1;
 void *x=array_data(a,n),*y=array_data(b,n);return x&&y&&!memcmp(x,y,n*4);
}
static int same_layout(void *a,void *b){
 if(memcmp((char*)a+12,(char*)b+12,8))return 0;
 void *ar=mp(a,8),*br=mp(b,8);if(!ar||!br)return ar==br;
 unsigned n=mw(ar,12);if(n!=mw(br,12)||n>MEMO_CHARS)return 0;
 if(!n)return 1;
 void *ap=array_data(ar,n),*bp=array_data(br,n);if(!ap||!bp)return 0;
 for(unsigned i=0;i<n;i++){
  void *x=mp(ap,i*4),*y=mp(bp,i*4);if(!x||!y||memcmp((char*)x+16,(char*)y+16,12))return 0;
  void *xc=mp(x,28),*yc=mp(y,28);if(!xc||!yc||memcmp((char*)xc+8,(char*)yc+8,16))return 0;
  if(!same_array(mp(x,8),mp(y,8))||!same_array(mp(x,12),mp(y,12)))return 0;
 }
 return 1;
}
void *text_memo_get(void *font,void *data,void *text,void *color){
 memo_started=1;memo_current=NULL;
 if(memo_disabled||!memo_create||!font||!data||!text||!color||!memo_data_init||!*memo_data_init||!memo_gc_stop||*memo_gc_stop)goto bypass;
 void *cache=mp(font,16);if(mm(cache,152)!=memo_clear||mm(cache,128)!=memo_draw)goto bypass;
 unsigned n=mw(text,8);if(!n||n>MEMO_CHARS)goto bypass;
 const uint16_t *chars=(const void*)((char*)text+12);
 /* Plain single-line text: no palette markup or line-breaking dependencies. */
 for(unsigned i=0;i<n;i++)if(chars[i]<32||chars[i]=='['||chars[i]==']'||(chars[i]>=0xd800&&chars[i]<=0xdfff))goto bypass;
 uintptr_t runs=mm(data,80),glyph=mm(data,72);void *packer=NULL;
 if(runs==memo_base_runs){if(glyph!=(uintptr_t)memo_glyph)goto bypass;}
 else if(runs==memo_ft_runs){
  if(glyph!=memo_ft_glyph||*((unsigned char*)data+128))goto bypass;
  packer=mp(data,120);if(packer&&mm(packer,216)!=(uintptr_t)memo_pack)goto bypass;
 }else goto bypass;
 /* Include common layout variants in the set selector. Equality still checks
  * the complete key below: this hash is never a validity shortcut. Without
  * these fields, every color/scale of a string fights for the same four slots. */
 uint32_t hash=(uint32_t)(uintptr_t)font^((uint32_t)(uintptr_t)data>>2);
 for(unsigned i=0;i<n;i++)hash=(hash^chars[i])*16777619u;
 const unsigned char *color_bytes=(const unsigned char*)color+8;
 const unsigned char *scale_bytes=(const unsigned char*)data+60;
 for(unsigned i=0;i<16;i++)hash=(hash^color_bytes[i])*16777619u;
 for(unsigned i=0;i<8;i++)hash=(hash^scale_bytes[i])*16777619u;
 hash^=hash>>16;
 unsigned first=(hash&((memo_active_slots/4)-1))*4,victim=first;MemoEntry *e;
 for(unsigned i=first;i<first+4;i++){
  e=&memo_entries[i];
  if(!e->valid||(memo_entries[victim].valid&&memo_entries[victim].age>e->age))victim=i;
  if(!e->valid||e->font!=font||e->data!=data||e->length!=n||memcmp(e->text,chars,n*2)||memcmp(e->color,(char*)color+8,16)||memcmp(e->data_state,(char*)data+8,96))continue;
  if(!glyphs_match(e,data,chars,n,0)){e->valid=0;e->vertex_generation++;victim=i;memo_invalid++;break;}
  /* FreeType.getGlyphs normally sets this flag even when no new glyph is made. */
  if(packer)memo_pack(packer,1);
  /* Periodically compare against a fresh original preparation. A mismatch
   * disables memoization immediately, before drawing this cached result. */
  if((memo_hits&255)==255){
   MemoEntry *check=&memo_entries[MEMO_SLOTS];
   if(!check->layout)check->layout=memo_create(NULL);
   if(!check->layout)goto bypass;
   memo_set(check->layout,font,text);memo_checks++;
   if(!same_layout(e->layout,check->layout)){memo_mismatch++;memo_disabled=1;goto bypass;}
  }
  e->age=++memo_clock;memo_hits++;memo_current=e;return e->layout;
 }
 if(!memo_rooted){memo_roots(memo_entries,memo_entries+MEMO_SLOTS+1);memo_rooted=1;}
 e=&memo_entries[victim];if(e->valid)memo_evict++;e->valid=0;e->vertex_generation++;
 /* Reject missing glyphs before preparation: original code may generate them. */
 if(!glyphs_match(e,data,chars,n,1))goto bypass;
 e->font=font;e->data=data;e->length=n;
 memcpy(e->text,chars,n*2);memcpy(e->color,(char*)color+8,16);memcpy(e->data_state,(char*)data+8,96);
 if(!e->layout)e->layout=memo_create(NULL);
 if(!e->layout)goto bypass;
 memo_set(e->layout,font,text);memo_misses++;e->age=++memo_clock;
 /* Preparation is allowed to change font internals. Store only if stable. */
 if(!memcmp(e->data_state,(char*)data+8,96)&&glyphs_match(e,data,chars,n,0))e->valid=1;
 if(e->valid)memo_current=e;
 return e->layout;
 bypass:memo_bypass++;return NULL;
}
#include "text_vertices.inc"
void text_memo_report(void){
 if(vertices_enabled)android_log_print(4,"TextVertices","hits=%u misses=%u bypass=%u checks=%u mismatch=%u disabled=%u slots=%u",vertex_hits,vertex_misses,vertex_bypass,vertex_checks,vertex_mismatch,vertices_disabled,vertex_active_slots);
 vertex_hits=vertex_misses=vertex_bypass=vertex_checks=vertex_mismatch=0;
 android_log_print(4,"TextMemo","hits=%u misses=%u bypass=%u invalid=%u evict=%u checks=%u mismatch=%u disabled=%u slots=%u",memo_hits,memo_misses,memo_bypass,memo_invalid,memo_evict,memo_checks,memo_mismatch,memo_disabled,memo_active_slots);
 memo_hits=memo_misses=memo_bypass=memo_invalid=memo_evict=memo_checks=memo_mismatch=0;
}
int text_memo_configure(ElfImage *im){
 const char *names[]={
 "_ZN3com8badlogic3gdx8graphics3g2d18GlyphLayout$_$CtorEPNS3_11GlyphLayoutE",
 "_ZN3com8badlogic3gdx8graphics3g2d25GlyphLayout$_setText_VoidEPNS3_11GlyphLayoutEPNS3_10BitmapFontEPN6System6StringE",
 "_ZN3com8badlogic3gdx8graphics3g2d52BitmapFont$BitmapFontData$_getGlyph_BitmapFont$GlyphEPNS3_25BitmapFont$BitmapFontDataEDs",
 "_ZN3com8badlogic3gdx8graphics3g2d34BitmapFont$Glyph$_getKerning_Int32EPNS3_16BitmapFont$GlyphEDs",
 "GC_add_roots",
 "_ZN3com8badlogic3gdx8graphics3g2d36PixmapPacker$_set_PackToTexture_VoidEPNS3_12PixmapPackerEb",
 "_ZN3com8badlogic3gdx8graphics3g2d41BitmapFont$BitmapFontData$_getGlyphs_VoidEPNS3_25BitmapFont$BitmapFontDataEPNS3_20GlyphLayout$GlyphRunEPN6System6StringEiib",
 "_ZN3com8badlogic3gdx8graphics3g2d8freetype60FreeTypeFontGenerator$FreeTypeBitmapFontData$_getGlyphs_VoidEPNS4_44FreeTypeFontGenerator$FreeTypeBitmapFontDataEPNS3_20GlyphLayout$GlyphRunEPN6System6StringEiib",
 "_ZN3com8badlogic3gdx8graphics3g2d8freetype71FreeTypeFontGenerator$FreeTypeBitmapFontData$_getGlyph_BitmapFont$GlyphEPNS4_44FreeTypeFontGenerator$FreeTypeBitmapFontDataEDs",
 "__$gc_stop_threads",
 "_ZZN6System8Internal12ClassHelpers9ClassCtorIN3com8badlogic3gdx8graphics3g2d10BitmapFontEEEvvE11_initalized",
 "_ZN3com8badlogic3gdx8graphics3g2d27BitmapFontCache$_clear_VoidEPNS3_15BitmapFontCacheE",
 "_ZN3com8badlogic3gdx8graphics3g2d26BitmapFontCache$_draw_VoidEPNS3_15BitmapFontCacheEPNS3_5BatchE"};
 uint32_t v[13],size;if(!im||!im->relocated)return -1;
 for(unsigned i=0;i<13;i++)if(elf_image_symbol(im,names[i],v+i,&size)||!elf_image_contains(im,v[i]&~1u,i==10?1:4,(i<9||i>10)?5:6))return -1;
 memo_create=(void*)(im->base+v[0]);memo_set=(void*)(im->base+v[1]);memo_glyph=(void*)(im->base+v[2]);memo_kerning=(void*)(im->base+v[3]);memo_roots=(void*)(im->base+v[4]);memo_pack=(void*)(im->base+v[5]);
 memo_base_runs=(uintptr_t)(im->base+v[6]);memo_ft_runs=(uintptr_t)(im->base+v[7]);memo_ft_glyph=(uintptr_t)(im->base+v[8]);memo_gc_stop=(void*)(im->base+v[9]);memo_data_init=im->base+v[10];memo_clear=(uintptr_t)(im->base+v[11]);memo_draw=(uintptr_t)(im->base+v[12]);return 0;
}
