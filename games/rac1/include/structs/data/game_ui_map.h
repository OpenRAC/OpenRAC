#ifndef STRUCTS_DATA_GAME_UI_MAP_H
#define STRUCTS_DATA_GAME_UI_MAP_H

#include "common.h"

typedef struct {
    int addr;            /* +0 */
    short unk4;          /* +4 */
    short cbp;           /* +6 */
    int unk8;            /* +8 */
    unsigned char tw;    /* +C */
    unsigned char th;    /* +D */
    short tbp;           /* +E */
} TexSlot;

#endif /* STRUCTS_DATA_GAME_UI_MAP_H */
