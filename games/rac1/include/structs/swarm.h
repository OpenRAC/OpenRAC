#ifndef STRUCTS_SWARM_H
#define STRUCTS_SWARM_H

#include "common.h"
#include "structs/vector.h"
#include "structs/records.h"

typedef struct {
    s32 count;
    u8 pad4[0xC];
    Vec4 points[1];
} SwarmPath;

typedef struct {
    s32 point;
    s8 direction;
    u8 pad5[0xB];
    SwarmPath *path;
    s32 path_id; /* index into D_L08_001B0CB0 */
    u8 pad18[0x18];
} SwarmPathFollower;

#endif /* STRUCTS_SWARM_H */
