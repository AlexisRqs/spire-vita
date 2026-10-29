#ifndef ELF_PROBE_H
#define ELF_PROBE_H
#include <stdint.h>
typedef struct {
 uint32_t load_segments, image_span, tls_memsz, tls_align;
 uint32_t tls_module_relocs, tls_offset_relocs, other_relocs;
} ElfProbe;
/* Metadata validation only: never invokes any game code. */
int elf_probe(const char *path, ElfProbe *out, char *error, unsigned error_size);
#endif
