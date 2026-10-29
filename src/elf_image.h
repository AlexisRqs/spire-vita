#ifndef ELF_IMAGE_H
#define ELF_IMAGE_H
#include <stdint.h>
#include "bionic_modules.h"
typedef struct { uint32_t start, size, flags; } ImageSegment;
typedef struct { unsigned char *data; uint32_t size; } ImageRelocs;
typedef struct {
 unsigned char *base;
 uint32_t capacity, count;
 ImageSegment segments[16];
 unsigned char *symbols, *strings;
 uint32_t symbols_size, strings_size;
 ImageRelocs relocs[8];
 uint32_t reloc_count, tls_size, tls_align, tls_filesz;
 unsigned char *tls_initial;
 int relocated;
 AndroidPhdr phdrs[64];
 uint32_t phnum;
} ElfImage;
/* Validates every initializer before returning offsets; never executes code. */
int elf_image_initializers(const ElfImage *,uint32_t load,uint32_t *offsets,unsigned capacity,unsigned *count);
typedef struct { uint32_t applied, tls, bound, unresolved; } RelocReport;
typedef int (*ImageResolve)(const char *name, uint32_t *address, void *context);
int elf_image_relocation_target(const ElfImage *im,const char *name,uint32_t type,uint32_t *target);
int elf_image_relocate(ElfImage *im, uint32_t load_address, ImageResolve resolve,
                       void *context, RelocReport *report, char *error, unsigned cap);
/* Caller owns a zero-based image buffer. Does not relocate or execute code. */
int elf_image_load(const char *path, void *buffer, uint32_t capacity,
                   ElfImage *image, char *error, unsigned error_size);
/* Optional startup-only index; tables must be immutable until release.
 * 0=ready, 1=allocation fallback, -1=invalid metadata. Not thread-safe. */
int elf_image_index_prepare(const ElfImage *image);
void elf_image_index_release(void);
int elf_image_symbol(const ElfImage *image, const char *name,
                     uint32_t *value, uint32_t *size);
int elf_image_contains(const ElfImage *image, uint32_t address, uint32_t size,
                       uint32_t required_flags);
#endif
