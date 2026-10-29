#ifndef ETC2_DECODE_H
#define ETC2_DECODE_H
#include <stddef.h>
#include <stdint.h>
/* Decode ETC2 RGB (9274) / punch-through alpha (9276) to tightly packed RGBA.
 * Returns -1 for invalid arguments/input, 0 on success. No allocation. */
int etc2_decode_rgba(unsigned format,unsigned width,unsigned height,const void *input,size_t input_size,void *output,size_t output_size);
#endif
