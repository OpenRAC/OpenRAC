#ifndef STRUCTS_VOICE_H
#define STRUCTS_VOICE_H

#include "common.h"

struct Moby;

/* What `voice_pool + i * 0x70` points at: 0x70 bytes before voice slot i, then the slot. The pool's own state fills the
 * first 0x70 bytes, so slot i really starts at 0x70 past that address. */
typedef struct VoicePoolRow {
    u8 before_slot[0x70];
    u32 handle;
    u8 state;
    u8 flags;
    u8 pad76[2];
    void *definition;
    u16 source_value;
    s16 linked_index;
    s32 volume;
    s32 pitch_bend;
    struct Moby *owner;
    s32 reserved8C;
    Vec4f position;
} VoicePoolRow;

#endif /* STRUCTS_VOICE_H */
