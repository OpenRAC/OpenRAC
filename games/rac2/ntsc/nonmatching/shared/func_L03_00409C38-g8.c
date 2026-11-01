/* func_L03_00409C38 (near miss, level 03 endako, 240 bytes at 3 places).
 *
 * Checked in a dedicated G8 unit (src/levels/shared/L03_00409C38-g8.c) built
 * with
 *   -O2 -G8 -ffunction-sections -frac2-qcopy
 * (the file needs its own flags: -frac2-qcopy is what lowers the quadword copy
 * to the grouped lq/sq pair with register addresses that retail has, and it
 * keeps the copy out of the other functions of the unit).
 *
 * With that profile the candidate is retail's exact 240 bytes and the whole
 * prologue, the two flag updates, the three calls, the state copy and the
 * epilogue are retail's word for word, in retail's order.  What is left is in
 * the float block:
 *
 *   - retail loads the two $gp floats into $f2 and $f1 (the second load after
 *     the 1/3600 constant is materialised), this cc1 loads them into $f3 and
 *     $f2 and loads the second one before the constant;
 *   - retail stores the angle (swc1 $f0,0x18($s0)) between the two
 *     multiplies, this cc1 stores it before both;
 *   - retail loads the call's third float argument in the call's delay slot
 *     (lwc1 $f12,0x8($s1)), this cc1 loads it before the call and puts the
 *     angle's store in the delay slot.
 *
 * The two $gp floats are at 0x1B1654 and 0x1B1658 (the $gp offsets 0x2664 and
 * 0x2668).  The constants are the 1/3600 and the 1/60 factors (0x3991A2B4 and
 * 0x3C888889).
 *
 * Tried and measured: the globals as memory operands, as locals, with the
 * constants as locals or as literals, the multiply operands swapped, the
 * rec[8] value in a local, the angle through a local, and the two globals read
 * on either side of the angle update.  The one that comes closest reads the
 * globals into locals inside the branch and keeps the constants as literals
 * (global1 into $f2 as retail has it), but the second global then lands in
 * $f3 instead of $f1.
 */

typedef unsigned char u8;

typedef unsigned int u32;

typedef unsigned short u16;

typedef float f32;

typedef int TI128 __attribute__((mode(TI)));

extern void func_002A7750(u8 *, int, int);

extern void func_002AA548(void *, void *, f32, f32, f32, f32);

extern void func_002A93B0(void *);

extern f32 D_L03_001B1654 __attribute__((sda));

extern f32 D_L03_001B1658 __attribute__((sda));

/* Ages the entity's own turn by ten degrees, hands the record's state the
   angles the shared factors make, and leaves the record's turn at four. */
void func_L03_00409C38(u8 *a0, u32 a1) {
    u8 *rec = *(u8 **)(a0 + 0x68);

    if (a0[0x20] != 2) {
        *(u16 *)(a0 + 0x34) = (u16)((*(u16 *)(a0 + 0x34) & 0xFFBE) | 0x1000);
        *(u32 *)(a0 + 0x98) = *(u32 *)(*(u8 **)(a0 + 0x24) + 0x10);
        func_002A7750(a0, 2, -1);
        *(TI128 *)(a0 + 0x10) = *(TI128 *)rec;
        *(u32 *)(rec + 0x10) = 0;
        if (a1 == 0) {
            f32 g1 = D_L03_001B1654;
            f32 g2 = D_L03_001B1658;

            *(f32 *)(a0 + 0x18) = *(f32 *)(a0 + 0x18) + 10.0f;
            func_002AA548(a0 + 0x18, rec + 0x10, *(f32 *)(rec + 8), 1.0f,
                          g1 * 0.00027777778f, g2 * 0.016666668f);
        }
    }
    *(u16 *)(a0 + 0x34) = (u16)(*(u16 *)(a0 + 0x34) | 0x400);
    func_002A93B0(a0);
    *(u32 *)(rec + 0x14) = 4;
}
