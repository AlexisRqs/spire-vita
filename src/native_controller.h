#ifndef NATIVE_CONTROLLER_H
#define NATIVE_CONTROLLER_H
#include "elf_image.h"
#define VITA_VIRTUAL_L2 (1u<<24)
#define VITA_VIRTUAL_R2 (1u<<25)
typedef struct {float axes[6];uint32_t buttons;} NativeControllerState;
void *native_controller_open(int index);
void native_controller_get(void *handle,NativeControllerState *out);
void native_controller_sample(uint32_t vita_buttons,unsigned lx,unsigned ly,unsigned rx,unsigned ry,int blocked);
int native_controller_active(void);
/* Includes analog axes for the keyboard's release-to-neutral gate. */
int native_controller_input_held(uint32_t buttons,unsigned lx,unsigned ly,unsigned rx,unsigned ry);
int native_controller_configure(ElfImage *im);
#endif
