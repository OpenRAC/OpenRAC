/* NON_MATCHING func_L00_002901A8 -- src/overlays/shared/space_0028FB78.c
 * Best so far: BYTES 70/240 (70.8% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Camera-follow update: clears D_0013E130+0x2A, sets +0x2C=1, then if D_0015EE84 is in range and its per-mode fl
 *   Best is p3.c/p2.c: branch layout and the second half match; the only difference is the address of D_0013E130. 
 *   Would unblock: a wording that stops CSE from sharing the low part between the first part and the body, while s
 */
typedef struct { char p0[0x20]; s32 x20; s16 x24; s16 p26; s16 p28; s16 x2a; s16 x2c; } E_eed0;
extern E_eed0 D_0013E130_901A8 __asm__("D_0013E130");
extern struct { int x0, x4, x8_c; } D_L00_00173F00_901A8 __asm__("D_L00_00173F00");
extern struct { char p[0x58]; int x58, x5c; } D_L00_0016C960_901A8 __asm__("D_L00_0016C960");
extern int D_L00_0016128C_901A8 __asm__("D_L00_0016128C") MACRO_ADDR;
extern int D_L00_0015F6A8 MACRO_ADDR;
extern float D_L00_0015F4FC MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern u8 D_0013DE4B NOT_SDA;
extern u8 D_0013D4F8[];
void func_002348B8(void);
void func_L00_0028FCA0(s32);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/gameplay_space_0028e8a0.c, FUN_L00_0028eed0. */
void func_L00_002901A8(void) {
    int m, u;
    D_0013E130_901A8.x2c = 1;
    if (D_0013E130_901A8.x2a != 0) D_0013E130_901A8.x2a = 0;
    m = D_0015EE84;
    if (m == 1 && D_0013DE4B == 0) { D_L00_0015F6A8 = 0; return; }
    if (m != 0) {
        if (m == 0xE) { if (D_0013D4F8[0] == 0) goto out; }
        if (m < 0x14) goto in;
    }
out:
    D_L00_0015F6A8 = 0; return;
in:
    D_0013E130_901A8.x24 = -1;
    D_L00_0015F6A8 = 6;
    D_L00_0015F4FC = 1.0f;
    D_0013E130_901A8.x20 = 0;
    func_002348B8();
    u = D_L00_0016128C_901A8 - 0x60000;
    D_L00_0016C960_901A8.x58 = D_L00_00173F00_901A8.x4 + u;
    D_L00_0016C960_901A8.x5c = D_L00_00173F00_901A8.x8_c + u;
    D_L00_0016128C_901A8 = u;
    func_L00_0028FCA0(8);
}
