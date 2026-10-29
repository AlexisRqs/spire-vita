#ifndef GRID_SCALAR_STATE_H
#define GRID_SCALAR_STATE_H
#include <stdint.h>
#define GRID_SCALAR_SLOTS 32
typedef struct {uint32_t cap,value,epoch;} GridScalarFlag;
typedef struct {GridScalarFlag flags[GRID_SCALAR_SLOTS];uint32_t used,epoch,blend_epoch,src,dst,depth_epoch,depth_write;int valid;} GridScalarState;
void grid_scalar_reset(GridScalarState *s);
int grid_scalar_enable(GridScalarState *s,uint32_t cap,int enabled);
void grid_scalar_blend(GridScalarState *s,uint32_t src,uint32_t dst);
void grid_scalar_depth(GridScalarState *s,int enabled);
int grid_scalar_equal(const GridScalarState *a,const GridScalarState *b);
#endif
