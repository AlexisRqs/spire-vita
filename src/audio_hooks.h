#ifndef AUDIO_HOOKS_H
#define AUDIO_HOOKS_H
#include "elf_image.h"
#include "audio_runtime.h"
#include "audio_output.h"
/* Startup only, before any engine threads/calls. Atomic validation before edits.
 * Runtime/output remain alive until game callers stop (or process exit).
 * Disable stops new requests but does not destroy referenced objects. */
int audio_hooks_configure(ElfImage *,AudioRuntime *,AudioOutput *);
void audio_hooks_disable(void);
void audio_hooks_poll(void);
#endif
