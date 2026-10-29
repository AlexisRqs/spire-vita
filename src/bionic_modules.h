#ifndef BIONIC_MODULES_H
#define BIONIC_MODULES_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
 uint32_t type,offset,vaddr,paddr,filesz,memsz,flags,align;
} AndroidPhdr;
_Static_assert(sizeof(AndroidPhdr)==32,"ELF32 program header");
/* Pre-API30 dl_phdr_info. Native-width pointers enable host testing; ARM
 * compile assertions enforce the actual guest layout. Callback size is exact. */
typedef struct {
 uintptr_t address;
 const char *name;
 const AndroidPhdr *headers;
 uint16_t count;
} AndroidDlInfo;
#ifdef __arm__
_Static_assert(sizeof(AndroidDlInfo)==16,"Android ARM32 dl_phdr_info prefix");
_Static_assert(offsetof(AndroidDlInfo,count)==12,"Android phnum offset");
#endif
/* Registration copies metadata. Image memory remains caller-owned until a
 * successful removal. Removal returns EBUSY while an iterator pins the image. */
int bionic_module_add(uintptr_t base,size_t image_size,const char *name,
                      const AndroidPhdr *headers,uint16_t count,uint32_t *id);
/* Caller must keep image registered/alive throughout unwinding. */
uintptr_t bionic_dl_unwind_find_exidx(uintptr_t pc,int *count);
int bionic_module_remove(uint32_t id);
int bionic_dl_iterate_phdr(int (*callback)(AndroidDlInfo *,size_t,void *),void *data);
#endif
