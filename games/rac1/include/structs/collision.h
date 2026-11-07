#ifndef STRUCTS_COLLISION_H
#define STRUCTS_COLLISION_H

#include "common.h"
#include "structs/vector.h"
#include "structs/moby.h"

/* coll_hit_result: what the last collision test hit. */
struct CollisionHit {
    u8 pad0[0x18];
    s32 moby;     /* 0x18: the moby hit, or 0 */
    s32 unk1C;    /* 0x1C: > 0 means there is a hit */
    Vec4 point;   /* 0x20: collision_hit_point */
    Vec4 pushed;  /* 0x30: coll_hit_position, the position after push-out */
    Vec4 normal;  /* 0x40: the surface normal */
    Vec4 velocity_end;   /* 0x50 */
    Vec4 velocity_start; /* 0x60 */
};

typedef struct CollisionHit CollisionHit;

#endif /* STRUCTS_COLLISION_H */
