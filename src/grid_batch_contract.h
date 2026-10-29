#ifndef GRID_BATCH_CONTRACT_H
#define GRID_BATCH_CONTRACT_H
#include <stdint.h>
#define GRID_BATCH_BYTES 92u
/* Only for the exact SpriteBatch layout validated by the caller. Preparation is
 * pure: reject every change outside the six observed scalar/pointer words. */
typedef struct {uint32_t words[6];} GridBatchPatch;
int grid_batch_prepare(GridBatchPatch *out,const unsigned char first_before[GRID_BATCH_BYTES],const unsigned char first_after[GRID_BATCH_BYTES],const unsigned char incoming[GRID_BATCH_BYTES]);
/* Caller must have validated object identity, frame, GC and unchanged incoming
 * state before GPU replay. No other bytes or referenced objects are modified. */
void grid_batch_apply(unsigned char batch[GRID_BATCH_BYTES],const GridBatchPatch *patch);
#endif
