#ifndef STRUCTS_DATA_GAME_UI_PAUSE_H
#define STRUCTS_DATA_GAME_UI_PAUSE_H

#include "common.h"

typedef struct {
    char b[0x20];
} Blk32;

typedef struct {
    int val;
    void *addr;
    int c1;
    int c2;
    int zero;
} Entry14;

typedef struct {
    unsigned short a; /* +0 */
    short b; /* +2 */
    short c; /* +4 */
    short id; /* +6 */
    short idx; /* +8 */
} Item0A;

typedef struct {
    unsigned short a; /* +0 */
    short b; /* +2 */
    int c; /* +4 */
    unsigned short d; /* +8 */
    short e; /* +A */
} Out0C;

typedef struct HandGadgetAnimation {
    s32 resource_first;
    s32 resource_count;
    s32 primary_animation;
    s32 delay_frames;
    s32 item_animation;
    s32 secondary_animation;
    s32 attachment0_class;
    s32 attachment0_animation;
    s32 attachment1_class;
    s32 attachment1_animation;
    s32 attachment2_class;
    s32 attachment2_animation;
} HandGadgetAnimation;

typedef struct {
    int unk00;
    int arg[13];
} Rec38;

#endif /* STRUCTS_DATA_GAME_UI_PAUSE_H */
