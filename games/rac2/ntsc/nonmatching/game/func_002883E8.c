/* func_002883E8 (near miss, boot 0x002883E8, 196 bytes, 28 places).
 *
 * The slot arm of the class-record updaters in game/002880E0.c.  Retail: if
 * the record's word at +0 is non-zero, its byte at +4 is zero and either its
 * word at +0x80 or its halfword at +0x88 is non-zero, walk the 0x20 bytes at
 * 0x1A7B68 and, for the first byte whose low six bits equal the id or which is
 * 0xFF, set result = 1 and store `id | (0x1A7AF8[id] ? 0x40 : 0)` there.
 *
 * Measured: -O2 -G0 -ffunction-sections, 196 bytes (exact size), 0 of 28
 * places.  Everything agrees except a cyclic rotation of the temporaries that
 * retail allocates around the loop:
 *
 *            retail            candidate
 *   result   $t0               $t2
 *   0xFF     $t1               $t0
 *   0x40     $t2               $t1
 *   &flag    $a1               $a3
 *   end      $a3               $v1
 *   byte     $v1               $a1
 *
 * The shapes that do agree are worth keeping for the next attempt:
 *   - a `short *` walk with `p += 5` and a `(int)p < (int)D_001A7B88`
 *     comparison, which is what makes retail emit `slt` rather than `sltu`;
 *   - a separate `unsigned int mask = cond ? 0x40 : 0;` statement, which is
 *     what makes this cc1 emit the `movz $v0,$zero,$byte` (writing the
 *     ternary inline into `id | (...)` instead folds it into two stores);
 *   - the do/while with the continue increment in the `bnel` delay slot.
 * Reordering declarations (`result` first) and an unused-flag draft changed
 * neither the size nor the rotation.  A fourth wording will not: this is the
 * documented "three wordings, same bytes" tie and needs a register-allocation
 * lever, not another spelling.
 *
 * M16: D_001A7AF8, D_001A7B68 and D_001A7B88 declared MACRO_ADDR (retail's lui/addiu in one
 * register, never in a slot) fix the forms and slots: 21 -> 10 lines. Left: result in $t2
 * where retail has $t0 (the 0xFF/0x40 constants take $t0/$t1), and the preheader order of
 * the 0xFF/0x40 constants against the D_001A7B68 address. Declaration and statement orders,
 * a byte local, int mask, 255 and a for loop leave it. In the permuter queue.
 */

#include "common.h"

typedef struct {
    int word00;
    unsigned char byte04;
    unsigned char pad05[7];
    int word0C;
    unsigned char pad10[0x70];
    int word80;
    unsigned char pad84[4];
    unsigned short half88;
    unsigned char pad8A[8];
    unsigned short half92;
    unsigned char pad94[0x4C];
} ClassRecord;

extern ClassRecord D_00239BA0[];

extern unsigned char D_00139568[];

extern unsigned char D_001A7AF8[] MACRO_ADDR;

extern unsigned char D_001A7B68[] MACRO_ADDR;

extern unsigned char D_001A7B88[] MACRO_ADDR;

int func_002883E8(int a0)
{
    int result = 0;
    ClassRecord *record = &D_00239BA0[D_00139568[a0]];
    unsigned char *p;

    if (record->word00 != 0 && record->byte04 == 0
        && (record->word80 != 0 || record->half88 != 0)) {
        p = D_001A7B68;
        do {
            if ((*p & 0x3F) == a0 || *p == 0xFF) {
                unsigned int mask = D_001A7AF8[a0] ? 0x40 : 0;
                result = 1;
                *p = a0 | mask;
                break;
            }
            p++;
        } while ((int)p < (int)D_001A7B88);
    }
    return result;
}
