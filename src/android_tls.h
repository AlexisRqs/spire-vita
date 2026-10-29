#ifndef ANDROID_TLS_H
#define ANDROID_TLS_H
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

/* A single ELF TLS module. Initialize before creating threads; destroy only
 * after all participating threads have exited. Not yet wired to the engine. */
typedef struct {
 pthread_key_t key;
 unsigned char *initial;
 size_t size, align;
 int active;
} AndroidTls;
typedef struct { uint32_t module, offset; } AndroidTlsIndex;
int android_tls_init(AndroidTls *tls, const void *initial, size_t filesz,
                     size_t memsz, size_t align);
void *android_tls_address(AndroidTls *tls, const AndroidTlsIndex *index);
void android_tls_destroy(AndroidTls *tls);
int android_tls_selftest(void);
int android_tls_link_test(void *(*resolve)(const AndroidTlsIndex *), const AndroidTlsIndex *index);
#endif
