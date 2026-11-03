#ifndef STRUCTS_HIT_MESSAGE_H
#define STRUCTS_HIT_MESSAGE_H

#include "structs/vector.h"

/* What MobyGetHitMessage returns: the hit linked from the moby's hit_slot. */
struct HitMessage {
    f32 unk_00;     /* 0x00 */
    f32 unk_04;     /* 0x04 */
    u8 pad_08[8];
    Vec4 unk_10;    /* 0x10 */
    char *source;   /* 0x20: the moby that sent it */
    s32 flags;      /* 0x24 */
    u8 unk_28;      /* 0x28 */
    u8 pad_29;
    s16 unk_2A;     /* 0x2A */
    f32 damage;     /* 0x2C: passed to hero_change_health */
    s32 unk_30;     /* 0x30 */
    char *target;   /* 0x34: the moby it is for */
    u8 pad_38[8];
};

#endif /* STRUCTS_HIT_MESSAGE_H */
