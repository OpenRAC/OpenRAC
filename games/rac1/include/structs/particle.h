#ifndef STRUCTS_PARTICLE_H
#define STRUCTS_PARTICLE_H

#include "structs/vector.h"

/* The record every particle type starts with (create_part returns one, KillPart frees it).
   The type's own data starts at 0x20; each type reads it through its own view. */
struct PartBase {
    u8 unk_00;     /* 0x00 */
    u8 unk_01;     /* 0x01: set to 0 at spawn */
    u8 texture;    /* 0x02: from the type's texture table */
    u8 blend;      /* 0x03: 0x48 or 0x44 */
    s32 color;     /* 0x04: RGBA, alpha in the top byte */
    u8 spin;       /* 0x08: rotation as a byte angle */
    u8 unk_09;     /* 0x09: set to (s32)4.0f - 0x60 at spawn */
    s16 timer;     /* 0x0A: ticks left (FastDecTimer) */
    f32 size;      /* 0x0C */
    Vec4 pos;      /* 0x10 */
    u8 data[0x40]; /* 0x20: the type's own data */
};

#endif /* STRUCTS_PARTICLE_H */
