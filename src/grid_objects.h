#ifndef GRID_OBJECTS_H
#define GRID_OBJECTS_H
#include "elf_image.h"
#include "grid_batch_contract.h"
/* Shallow observation plus guarded preparation of exact batch postconditions.
 * Card and GPU validation are independent prerequisites for reuse. */
int grid_objects_configure(ElfImage *im);
void grid_objects_begin(void *grid,void *batch,unsigned pass,unsigned frame,int sample);
void grid_objects_end(void);
int grid_objects_plain(void *grid,void *batch);
int grid_objects_prepare_replay(void *grid,void *batch,unsigned frame,GridBatchPatch *patch);
#endif
