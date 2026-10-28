#ifndef MEMCARD_H
#define MEMCARD_H

#include "common.h"

typedef struct McSaveSlot {
    s32 id;           /* 0x00: save slot index 0..4, or -1 if empty */
    s32 bolts;        /* 0x04 */
    s32 gold_bolts;   /* 0x08: count of collected gold bolts */
    s32 time;         /* 0x0C: play time in ticks (hours = time / 216000, minutes = (time / 3600) % 60) */
    union {
        char bytes[8];
        struct {
            u8 pad_0[5];
            u8 b15;
            u8 b16;
            u8 b17;
        } date;
    } name;           /* 0x10 */
    s32 valid;        /* 0x18: 1 if valid, 0 otherwise */
} McSaveSlot;         /* 0x1C */

typedef struct McCard {
    s32 port;                 /* 0x00 */
    s32 slot;                 /* 0x04 */
    s32 type;                 /* 0x08: 2 for PS2 memory card */
    s32 free;                 /* 0x0C: free blocks */
    s32 format;               /* 0x10: formatted state */
    s32 x14;                  /* 0x14: card error status */
    s32 x18;                  /* 0x18 */
    s32 result;               /* 0x1C: last card result (-2 unformatted, 0 formatted, -1 error) */
    McSaveSlot slots[5];      /* 0x20: 5 save slots, 0x1C bytes each */
    s32 used_blocks;          /* 0xAC */
    s32 request;              /* 0xB0: card request status (0 idle, 1 pending, 2 active) */
    s32 xB4;                  /* 0xB4 */
    s32 xB8;                  /* 0xB8 */
    s32 xBC;                  /* 0xBC */
} McCard;                     /* 0xC0 */

typedef struct MemoryCardState {
    McCard card[1];           /* 0x00: primary card (0xC0 bytes) */
    s32 cmd;                  /* 0xC0 */
    s32 result;               /* 0xC4 */
    s32 xC8;                  /* 0xC8 */
    s32 cur;                  /* 0xCC: current card/slot index */
    s32 xD0;                  /* 0xD0 */
    s32 busy;                 /* 0xD4: 1 if async op is busy */
    s32 fd;                   /* 0xD8: file descriptor */
    s32 state;                /* 0xDC: state machine state */
    s32 sub;                  /* 0xE0: sub-state */
    s32 err;                  /* 0xE4: error code (-1 if none) */
    s32 errCard;              /* 0xE8: error card (-1 if none) */
    void *buf;                /* 0xEC: buffer / error latch */
    s32 size;                 /* 0xF0: buffer size */
    s32 padF4;                /* 0xF4 */
    s32 padF8;                /* 0xF8 */
    s32 field_fc;             /* 0xFC: flag passed to mode_freeze_init */
} MemoryCardState;            /* 0x100 */

#endif /* MEMCARD_H */
