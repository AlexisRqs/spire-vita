#ifndef DIRECT_CACHE_H
#define DIRECT_CACHE_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
/* Immutable installation cache. Caller serializes init/shutdown with reads.
 * Reads are serialized internally. Returned KTX allocation belongs to caller. */
int direct_cache_init(const char *index,const char *pack,const char *receipts);
void direct_cache_shutdown(void);
void *direct_cache_read(const char *name,FILE *source,size_t *size);
void direct_cache_report(void);
/* Path lifetime must cover startup. NULL disables the optional subset. */
void direct_cache_loading_path(const char *path);
void direct_cache_loading_begin(void);
void direct_cache_loading_end(void);
#endif
