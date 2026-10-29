#ifndef SPIRE_KEYBOARD_H
#define SPIRE_KEYBOARD_H
#include "elf_image.h"
#define KEYBOARD_CAPACITY 128
typedef struct {
 int (*begin)(const uint16_t *,const uint16_t *,uint16_t *,unsigned);
 int (*poll)(void); /* 0 running, 1 accepted, 2 cancelled, negative error */
 void (*end)(void);
} KeyboardBackend;
void keyboard_bind(KeyboardBackend backend);
void keyboard_show(const uint16_t *title,const uint16_t *initial);
int keyboard_update(const uint16_t **output);
int keyboard_active(void);
int keyboard_gate_input(int held);
int keyboard_configure(ElfImage *image);
void vita_keyboard_bind(void);
#endif
