/* Near miss from src/levels/15_gorn/L15_00315428-g8.c: the build refuses this body at its
   place — 176 bytes, expected 164. Kept as a starting point;
   not linked, not counted. */

/* Standalone declarations recovered from src/levels/15_gorn/L15_00315428-g8.c. */
typedef unsigned char      u8;

extern int D_L15_001B1520 __attribute__((sda));

extern int D_L15_001B1524;

extern int D_L15_001B1528;

extern int D_L15_001B152C;

extern u8 D_L15_001395B8[];

void func_L15_00432008(void)
{
    if (D_L15_001B1520 >= 0x80) {
        D_L15_001B1520 = 0x7F;
    }
    if (D_L15_001B1524 >= 0x80) {
        D_L15_001B1524 = 0x7F;
    }
    if (D_L15_001B1528 >= 0x80) {
        D_L15_001B1528 = 0x7F;
    }
    if (D_L15_001B152C >= 0x80) {
        D_L15_001B152C = 0x7F;
    }
    D_L15_001395B8[0x6B] = *(u8 *)&D_L15_001B1520;
    D_L15_001395B8[0x6C] = *(u8 *)&D_L15_001B1524;
    D_L15_001395B8[0x6D] = *(u8 *)&D_L15_001B1528;
    D_L15_001395B8[0x6E] = *(u8 *)&D_L15_001B152C;
}

/* ---------------------------------------------------------------------------
 * wave 24, worker 14: the same shape as the attempt above (the four clamps and
 * the four byte copies), 176 bytes against retail's 164.  The four clamps are
 * right, but this cc1 loads the $gp global D_L15_001B1520 twice before the
 * store (retail loads it once) and the four byte copies come out in the order
 * 0x6B, 0x6D, 0x6C, 0x6E where retail has 0x6B, 0x6E, 0x6C, 0x6D.  The
 * register assignment of the four copies matches (a0, a2, a1, v1).
 */

extern int D_L15_001B1520 __attribute__((sda));
extern int D_L15_001B1524;
extern int D_L15_001B1528;
extern int D_L15_001B152C;
extern u8 D_L15_001395B8[];

void func_L15_00432008_w24(void)
{
    if (D_L15_001B1520 >= 0x80) {
        D_L15_001B1520 = 0x7F;
    }
    if (D_L15_001B1524 >= 0x80) {
        D_L15_001B1524 = 0x7F;
    }
    if (D_L15_001B1528 >= 0x80) {
        D_L15_001B1528 = 0x7F;
    }
    if (D_L15_001B152C >= 0x80) {
        D_L15_001B152C = 0x7F;
    }
    D_L15_001395B8[0x6B] = *(u8 *)&D_L15_001B1520;
    D_L15_001395B8[0x6C] = *(u8 *)&D_L15_001B1524;
    D_L15_001395B8[0x6D] = *(u8 *)&D_L15_001B1528;
    D_L15_001395B8[0x6E] = *(u8 *)&D_L15_001B152C;
}
