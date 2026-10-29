#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H
#include <stdint.h>
#include <stddef.h>
typedef struct {
 uint64_t source_hash_us,lookup_us,read_us,checksum_us,total_us,wait_us;
 uint64_t source_bytes,payload_bytes;
 unsigned attempts,hits;
} TextureCacheProfile;
/* Atomically retrieve and reset interval counters; does not close the pack. */
TextureCacheProfile texture_cache_profile_take(void);
/* Reset before replacing immutable pack files; serializes with loads. */
void texture_cache_reset(void);
uint64_t texture_cache_hash(const void *data,size_t n);
/* Optional derived files, owned payload returned on success; caller frees. */
void *texture_cache_load(const char *root,unsigned format,unsigned width,unsigned height,const void *source,size_t size,unsigned *out_width,unsigned *out_height,size_t *out_size);
#endif
