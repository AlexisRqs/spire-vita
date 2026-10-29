#include "android_tls.h"
#include <stdlib.h>
#include <string.h>

typedef struct { unsigned char *data; } TlsBlock;

int android_tls_init(AndroidTls *tls, const void *initial, size_t filesz,
                     size_t memsz, size_t align) {
 if (!tls || !memsz || memsz > 4096 || filesz > memsz ||
     (filesz && !initial) || !align || align > 4096 || (align & (align-1)))
  return -1;
 memset(tls, 0, sizeof(*tls));
 tls->initial = calloc(1, memsz);
 if (!tls->initial) return -1;
 if (filesz) memcpy(tls->initial, initial, filesz);
 if (pthread_key_create(&tls->key, free)) {
  free(tls->initial); tls->initial = NULL; return -1;
 }
 tls->size = memsz; tls->align = align; tls->active = 1;
 return 0;
}

void *android_tls_address(AndroidTls *tls, const AndroidTlsIndex *index) {
 if (!tls || !tls->active || !index || index->module != 1 ||
     index->offset >= tls->size) return NULL;
 TlsBlock *block = pthread_getspecific(tls->key);
 if (!block) {
  block = malloc(sizeof(*block) + tls->align - 1 + tls->size);
  if (!block) return NULL;
  uintptr_t start = (uintptr_t)(block + 1);
  block->data = (unsigned char *)((start + tls->align - 1) &
                                  ~(uintptr_t)(tls->align - 1));
  memcpy(block->data, tls->initial, tls->size);
  if (pthread_setspecific(tls->key, block)) { free(block); return NULL; }
 }
 return block->data + index->offset;
}

void android_tls_destroy(AndroidTls *tls) {
 if (!tls || !tls->active) return;
 /* pthread_key_delete does not invoke the destructor for the caller. */
 free(pthread_getspecific(tls->key));
 pthread_setspecific(tls->key, NULL);
 pthread_key_delete(tls->key);
 free(tls->initial);
 memset(tls, 0, sizeof(*tls));
}
