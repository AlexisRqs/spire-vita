#ifndef GRID_COMMANDS_H
#define GRID_COMMANDS_H
#include <stddef.h>
#include <stdint.h>
/* Private, frame-local command storage. No borrowed payload pointers. Integration
 * must additionally validate incoming GL and managed state before replay. */
typedef enum {
 GRID_CMD_PROGRAM=1, GRID_CMD_TEXTURE, GRID_CMD_ACTIVE_TEXTURE,
 GRID_CMD_ENABLE, GRID_CMD_DISABLE, GRID_CMD_BLEND,
 GRID_CMD_UNIFORM_I, GRID_CMD_MATRIX4, GRID_CMD_ATTR_ENABLE,
 GRID_CMD_ATTR_DISABLE, GRID_CMD_TRIANGLES, GRID_CMD_DEPTH_MASK
} GridCommandOp;
typedef struct {
 unsigned char *bytes;size_t capacity,used;uint32_t frame,count;
 unsigned recording,valid,sealed;
} GridCommands;
typedef void (*GridCommandSink)(void *context,GridCommandOp op,const uint32_t args[4],const void *payload,size_t size);
void grid_commands_init(GridCommands *s,void *storage,size_t capacity);
void grid_commands_begin(GridCommands *s,uint32_t frame);
void grid_commands_invalidate(GridCommands *s);
int grid_commands_append(GridCommands *s,GridCommandOp op,const uint32_t args[4],const void *payload,size_t size);
int grid_commands_seal(GridCommands *s);
int grid_commands_equal(const GridCommands *a,const GridCommands *b,uint32_t frame);
/* Validates the entire stream before issuing the first callback. Sink must not
 * modify the stream. No callbacks occur for malformed/stale/unsealed streams. */
int grid_commands_replay(const GridCommands *s,uint32_t frame,GridCommandSink sink,void *context);
#endif
