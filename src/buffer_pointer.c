#include "buffer_pointer.h"
#include "compat_libc.h"
#include "android_log.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
/* Exact APK Buffer owns a pinned GCHandle for its immutable byte[] backing.
 * Constructors set backing; getPointer allocates the pin; dispose/finalize
 * free and clear it. For these exact types an allocated pin therefore refers
 * to backing+8, as in GetAddrOfPinnedObject. Never retain an object/data pointer
 * across calls, never read the global handle table without its mutex.
 * First pin, unknown types and GC safepoints always take the original path. */
uint32_t buffer_pointer_resume;
static volatile uint32_t *bp_gc_stop;
static uint32_t *bp_types[5],*bp_array_type;
static int bp_configured;
static atomic_uint bp_active,bp_fast,bp_bypass,bp_checks,bp_mismatch;
static uint32_t word(void *p,unsigned n){uint32_t v;memcpy(&v,(char*)p+n,4);return v;}
static void *ptr(void *p,unsigned n){return (void*)(uintptr_t)word(p,n);}
extern void *ANDROID_ABI buffer_pointer_original(void *buffer);
void *ANDROID_ABI buffer_pointer_hook(void *buffer){
 if(!atomic_load(&bp_active)||!buffer||!bp_gc_stop||*bp_gc_stop)goto original;
 uint32_t t=word(buffer,0);int known=0;
 for(unsigned i=0;i<5;i++)if(bp_types[i]&&*bp_types[i]&&t==*bp_types[i]+0x38){known=1;break;}
 if(!known||!word(buffer,20))goto original;
 void *a=ptr(buffer,16);
 if(!a||!bp_array_type||!*bp_array_type||word(a,0)!=*bp_array_type+0x38)goto original;
 void *p=ptr(a,8);if(!p)goto original;
 /* Sample the original result to detect a violated ownership contract. */
 unsigned n=atomic_fetch_add(&bp_fast,1);
 if(!(n&127)){
  void *expected=buffer_pointer_original(buffer);atomic_fetch_add(&bp_checks,1);
  if(expected!=p){atomic_fetch_add(&bp_mismatch,1);atomic_store(&bp_active,0);return expected;}
 }
 return p;
 original:atomic_fetch_add(&bp_bypass,1);return buffer_pointer_original(buffer);
}
static const struct {const char *name;uint32_t value,size,hash;} contract[]={
#include "buffer_pointer_contract.inc"
};
static uint32_t hash(const unsigned char*p,unsigned n){uint32_t h=2166136261u;for(unsigned i=0;i<n;i++)h=(h^p[i])*16777619u;return h;}
int buffer_pointer_configure(ElfImage *im){
 if(!im||!im->relocated||bp_configured)return -1;
 for(unsigned i=0;i<sizeof(contract)/sizeof(*contract);i++){
  uint32_t v,s;
  if(elf_image_symbol(im,contract[i].name,&v,&s)||v!=contract[i].value||s!=contract[i].size||!elf_image_contains(im,v&~1u,s,5)||hash(im->base+(v&~1u),s)!=contract[i].hash)return -1;
 }
 const char *names[]={
 "_ZZN6System8Internal14GetRuntimeTypeIPN4java3nio6BufferEEEPNS0_11RuntimeTypeEvE6cached",
 "_ZZN6System8Internal14GetRuntimeTypeIPN4java3nio10ByteBufferEEEPNS0_11RuntimeTypeEvE6cached",
 "_ZZN6System8Internal14GetRuntimeTypeIPN4java3nio11FloatBufferEEEPNS0_11RuntimeTypeEvE6cached",
 "_ZZN6System8Internal14GetRuntimeTypeIPN4java3nio9IntBufferEEEPNS0_11RuntimeTypeEvE6cached",
 "_ZZN6System8Internal14GetRuntimeTypeIPN4java3nio11ShortBufferEEEPNS0_11RuntimeTypeEvE6cached",
 "_ZZN6System8Internal14GetRuntimeTypeIPNS_6ArrayTIhLi1EEEEEPNS0_11RuntimeTypeEvE6cached",
 "__$gc_stop_threads"};
 uint32_t addresses[7];
 for(unsigned i=0;i<7;i++){uint32_t s;if(elf_image_symbol(im,names[i],addresses+i,&s)||s!=4||!elf_image_contains(im,addresses[i],4,6))return -1;}
 for(unsigned i=0;i<5;i++)bp_types[i]=(void*)(im->base+addresses[i]);
 bp_array_type=(void*)(im->base+addresses[5]);bp_gc_stop=(void*)(im->base+addresses[6]);
 uint32_t at=contract[0].value&~1u,target=(uint32_t)(uintptr_t)buffer_pointer_hook;
 buffer_pointer_resume=(uint32_t)(uintptr_t)(im->base+at+8)|1;
 const unsigned char jump[]={0xdf,0xf8,0,0xf0};memcpy(im->base+at,jump,4);memcpy(im->base+at+4,&target,4);
 bp_configured=1;return 0;
}
void buffer_pointer_enable(int active){if(bp_configured&&(!active||!atomic_load(&bp_mismatch)))atomic_store(&bp_active,active!=0);}
void buffer_pointer_report(void){
 if(!bp_configured)return;
 android_log_print(4,"BufferPointer","mode=%u fast=%u bypass=%u checks=%u mismatch=%u",atomic_load(&bp_active),atomic_exchange(&bp_fast,0),atomic_exchange(&bp_bypass,0),atomic_exchange(&bp_checks,0),atomic_load(&bp_mismatch));
 const char *path="ux0:data/spireandroid/buffer-pointer-command.txt";
 FILE *f=fopen(path,"r");if(!f)return;
 unsigned mode,serial;char extra;int n=fscanf(f,"buffer_pointer %u %u %c",&mode,&serial,&extra);int bad=ferror(f);fclose(f);
 if(remove(path))return;
 if(bad||n!=2||mode>1||atomic_load(&bp_mismatch)){atomic_store(&bp_active,0);return;}
 atomic_store(&bp_active,mode);android_log_print(4,"BufferPointerChange","mode=%u serial=%u",mode,serial);
}
