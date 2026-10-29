#ifndef ETC2_TRANSCODE_H
#define ETC2_TRANSCODE_H
#include <stddef.h>
/* Linear block-order ETC2 to DXT1 (RGB) / DXT5 (alpha).
 * Caller owns compressed output; scratch is one 64-byte RGBA tile.
 * No allocation, original dimensions preserved, partial tiles edge-clamped. */
int etc2_transcode(unsigned format,unsigned width,unsigned height,const void *input,size_t input_size,void *output,size_t output_size);
#endif
