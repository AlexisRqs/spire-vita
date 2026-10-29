#ifndef GRID_UNIFORMS_H
#define GRID_UNIFORMS_H
#include <stdint.h>
#define GRID_UNIFORM_SLOTS 256
typedef struct {uint32_t program;int32_t location;uint32_t kind,count,epoch;unsigned char value[256];} GridUniform;
typedef struct {GridUniform entries[GRID_UNIFORM_SLOTS];uint32_t used,epoch;int valid;} GridUniformState;
void grid_uniforms_reset(GridUniformState *s);
int grid_uniforms_integer(GridUniformState *s,uint32_t program,int32_t location,int32_t value);
int grid_uniforms_matrix(GridUniformState *s,uint32_t program,int32_t location,int count,int transpose,const void *value);
int grid_uniforms_sprite_program(const GridUniformState *s,uint32_t program,int matrix_location,int sampler_location);
int grid_uniforms_equal(const GridUniformState *a,const GridUniformState *b);
#endif
