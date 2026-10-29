/* Exact-position snapshots, only for privately owned, validated memo layouts.
 * The original populates the first snapshot. No transformed/approximate vertices.
 * Live managed objects remain rooted by the enclosing memo entry; stored
 * pointer bits are only compared, never dereferenced or restored. */
/* Four-way table keyed by validated memo generation AND exact position.
 * Snapshot pointers are compared only, never dereferenced or restored. The
 * live font/cache/layout remain owned by the rooted memo entry. */
typedef struct {MemoEntry *owner;unsigned generation,age,count;float x,y;unsigned char before[64];uint32_t tint;float vertices[MEMO_CHARS*20];} VertexEntry;
static VertexEntry vertex_entries[VERTEX_STORAGE_SLOTS];static unsigned vertex_clock;
void text_vertices_enable(int enabled){vertices_enabled=enabled;}
static void *vertex_array(void *a,unsigned min){
 if(!a)return NULL;
 void *shape=mp(a,20);
 if(!shape||mw(shape,0)<min)return NULL;
 return mp(a,8);
}
static int vertex_storage(void *cache,void **indices,void **verts,void **layouts,void **items,unsigned count){
 if(!cache||mp(cache,52)||*((unsigned char*)cache+12))return 0;
 void *font=mp(cache,8),*regions=font?mp(font,12):NULL;
 if(!regions||mw(regions,12)!=1)return 0;
 void *pages=mp(cache,44),*ix=mp(cache,48);
 if(!pages||!ix||!mp(pages,20)||!mp(ix,20)||mw(mp(pages,20),0)!=1||mw(mp(ix,20),0)!=1)return 0;
 void *pp=vertex_array(pages,1);*indices=vertex_array(ix,1);
 if(!pp||!*indices)return 0;
 *verts=vertex_array(mp(pp,0),count);
 *layouts=mp(cache,16);*items=*layouts?vertex_array(mp(*layouts,8),1):NULL;
 return *verts&&*items;
}
void ANDROID_ABI text_vertices_add(void *cache,void *layout,float x,float y,void (ANDROID_ABI *original)(void*,void*,float,float)){
 MemoEntry *e=memo_current;void *indices,*verts,*layouts,*items;
 if(!vertices_enabled||vertices_disabled||!e||!e->valid||layout!=e->layout||!memo_gc_stop||*memo_gc_stop||cache!=mp(e->font,16))goto bypass;
 for(unsigned i=0;i<e->length;i++)if(!e->glyph[i]||mw(e->glyph[i],64)!=0)goto bypass;
 if(!vertex_storage(cache,&indices,&verts,&layouts,&items,1)||mw(indices,0)||mw(layouts,12))goto bypass;
 unsigned char before[64];memcpy(before,cache,64);
 uint32_t xb,yb;memcpy(&xb,&x,4);memcpy(&yb,&y,4);
 uint32_t hash=(uint32_t)(e-memo_entries)*0x9e3779b9u;
 hash=(hash^xb)*16777619u;hash=(hash^yb)*16777619u;hash^=hash>>16;
 unsigned first=(hash&((vertex_active_slots/4)-1))*4,victim=first;VertexEntry *v=NULL;
 for(unsigned i=first;i<first+4;i++){
  VertexEntry *c=&vertex_entries[i];
  if(!c->owner||(vertex_entries[victim].owner&&c->age<vertex_entries[victim].age))victim=i;
  if(c->owner==e&&c->generation==e->vertex_generation&&!memcmp(&x,&c->x,4)&&!memcmp(&y,&c->y,4)&&!memcmp(before,c->before,64)){v=c;break;}
 }
 if(v&&vertex_storage(cache,&indices,&verts,&layouts,&items,v->count)){
  if((vertex_hits&255)==255){
   original(cache,layout,x,y);vertex_checks++;
   if(!vertex_storage(cache,&indices,&verts,&layouts,&items,v->count)||memcmp(cache,v->before,40)||memcmp((char*)cache+44,v->before+44,20)||mw(cache,40)!=v->tint||mw(indices,0)!=v->count||mw(layouts,12)!=1||mp(items,0)!=layout||memcmp(verts,v->vertices,v->count*4)){
    vertices_disabled=1;vertex_mismatch++;return;
   }
  }else{
   memcpy(verts,v->vertices,v->count*4);
   uint32_t p=(uint32_t)(uintptr_t)layout,one=1;
   memcpy(items,&p,4);memcpy((char*)layouts+12,&one,4);
   memcpy(indices,&v->count,4);memcpy((char*)cache+40,&v->tint,4);
  }
  v->age=++vertex_clock;vertex_hits++;return;
 }
 original(cache,layout,x,y);vertex_misses++;
 if(memcmp(before,cache,40)||memcmp(before+44,(char*)cache+44,20))return;
 if(!vertex_storage(cache,&indices,&verts,&layouts,&items,1)||mw(layouts,12)!=1||mp(items,0)!=layout)return;
 unsigned count=mw(indices,0);if(!count||count>MEMO_CHARS*20||count%20||!vertex_storage(cache,&indices,&verts,&layouts,&items,count))return;
 v=&vertex_entries[victim];v->owner=e;v->generation=e->vertex_generation;v->age=++vertex_clock;
 memcpy(v->before,before,64);v->tint=mw(cache,40);memcpy(v->vertices,verts,count*4);
 v->x=x;v->y=y;v->count=count;return;
 bypass:
 if(vertices_enabled&&cache&&e&&e->valid&&vertex_bypass==0){
  void *font=mp(cache,8),*regions=font?mp(font,12):NULL,*pa=mp(cache,44),*ix=mp(cache,48),*la=mp(cache,16);
  android_log_print(4,"VertexGuard","integer=%u page_indices=%u temp_counts=%u regions=%u pages=%u indices=%u layouts=%u",*((unsigned char*)cache+12),mp(cache,52)!=NULL,mp(cache,56)!=NULL,regions?mw(regions,12):999,pa&&mp(pa,20)?mw(mp(pa,20),0):999,ix&&mp(ix,20)?mw(mp(ix,20),0):999,la?mw(la,12):999);
 }
 vertex_bypass++;original(cache,layout,x,y);
}
