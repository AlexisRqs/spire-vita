#ifndef GC_PROBE_H
#define GC_PROBE_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
 void (*init)(void);
 void (*allow_threads)(void);
 void *(*allocate)(size_t);
 void (*add_roots)(void *,void *);
 void (*finalizer)(void *,void (*)(void *,void *),void *,void (**)(void *,void *),void **);
 void (*collect)(void);
 int (*invoke_finalizers)(void);
 volatile uint32_t *counter;
} GCProbeApi;
/* One process-lifetime probe. The engine/image must remain resident afterwards.
 * Returns 10 if conservative roots postpone finalization beyond the retry bound. */
int gc_probe_run(const GCProbeApi *api,void (*stage)(const char *,uint32_t));
#endif
