/* Carried from rac3-decomp (src/levels/shared/L01_004B1160.c, func_L01_004B11E0): the
   same instructions. Sends N 16-byte records from BUF to the record table IDX (of 64),
   starting at record START, in requests of at most 0xFFFF records through func_0011D620.
   N == -1 means all of them (func_002EEFE0). -3 for an empty or out-of-range table, -1
   when the records would run past its end. */

#include "common.h"

typedef struct {
    u8 *base;
    s32 count;
    s32 pad[2];
} RecordTable;

extern RecordTable D_00215050[];
extern s32 D_001A7248 MACRO_ADDR;
extern s32 func_002EEFE0(u32);
extern s32 func_0011D620(void *, s32, s32, u8 **, s32, u8 *, s32, s32, s32);

s32 func_002EEED0(u8 *buf, u32 idx, s32 start, s32 n, s32 arg) {
    u8 *dst;
    s32 c;
    s32 sz;
    if (n == -1) {
        n = func_002EEFE0(idx);
    }
    if (idx >= 64 || D_00215050[idx].count == 0) {
        return -3;
    }
    if (D_00215050[idx].count < start + n) {
        return -1;
    }
    dst = D_00215050[idx].base + start * 16;
    while (n != 0) {
        c = n;
        if (c > 0xFFFF) {
            c = 0xFFFF;
        }
        sz = c * 16;
        func_0011D620(&D_001A7248, 1, arg, &dst, 16, buf, sz, 0, 0);
        n -= c;
        buf += sz;
        dst += sz;
    }
    return 0;
}
