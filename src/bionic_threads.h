#ifndef BIONIC_THREADS_H
#define BIONIC_THREADS_H
#include <stdint.h>
#include <stddef.h>
/* Fixed Android ARM32 layout, including on a 64-bit test host. */
typedef struct { uint32_t flags,stack_base,stack_size,guard_size; int32_t policy,priority; } BionicAttr;
_Static_assert(sizeof(BionicAttr)==24,"Android ARM32 pthread_attr_t size");
int bionic_thread_stack(uint32_t id,uintptr_t *base,size_t *size);
int bionic_stack_to_attr(uintptr_t base,size_t size,BionicAttr *out);
uintptr_t bionic_threads_lookup(const char *name);
unsigned bionic_threads_live(void);
#endif
