#ifndef STRUCTS_GS_H
#define STRUCTS_GS_H

#include "common.h"
#include "structs/vector.h"
#include "structs/records.h"

typedef struct {
    unsigned long NLOOP : 15;
    unsigned long EOP : 1;
    unsigned long pad16 : 16;
    unsigned long id : 14;
    unsigned long PRE : 1;
    unsigned long PRIM : 11;
    unsigned long FLG : 2;
    unsigned long NREG : 4;
    unsigned long REGS0 : 4;
    unsigned long REGS1 : 4;
    unsigned long REGS2 : 4;
    unsigned long REGS3 : 4;
    unsigned long REGS4 : 4;
    unsigned long REGS5 : 4;
    unsigned long REGS6 : 4;
    unsigned long REGS7 : 4;
    unsigned long REGS8 : 4;
    unsigned long REGS9 : 4;
    unsigned long REGS10 : 4;
    unsigned long REGS11 : 4;
    unsigned long REGS12 : 4;
    unsigned long REGS13 : 4;
    unsigned long REGS14 : 4;
    unsigned long REGS15 : 4;
} GifTag_FAB40;

typedef struct {
    unsigned long FBP : 9;
    unsigned long rest : 55;
} GsFbp_FAB40;

typedef struct {
    unsigned long pmode;
    unsigned long smode2;
    GsFbp_FAB40 dispfb;
    unsigned long display;
    unsigned long bgcolor;
    unsigned long pad028;
    GifTag_FAB40 giftag0;       /* 0x030 */
    GsFbp_FAB40 frame0;         /* 0x040 */
    unsigned long draw0[15];
    GifTag_FAB40 giftag1;       /* 0x0C0 */
    GsFbp_FAB40 frame1;         /* 0x0D0 */
    unsigned long draw1[15];
    short display_width;        /* 0x150 */
    short display_height;
    short psm;
    short fbp0;
    short storage_width;        /* 0x158 */
    short storage_height;
    short storage_psm;
    short fbp1;
    short target_width;         /* 0x160 */
    short target_height;
    short target_psm;
    short target_fbp;
    short display_offset_x;     /* 0x168 */
    short display_offset_y;
    short zpsm;
    short zbp;
    int reserved170;            /* 0x170 */
} FsAa_FAB40;

#endif /* STRUCTS_GS_H */
