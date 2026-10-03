/* NON_MATCHING func_L00_002584A8 -- src/overlays/shared/mobyproc_00251A78.c
 * Best so far: SIZE ours 856 / retail 852, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   SetDeathBits: sets the moby's bit in three per-level save tables (D_0014171B+0xAB75, D_L00_001BA860, slot byte
 *   Best candidate p7.c (size matches, 338 bytes differ): lvl local (gp read of D_0015EE84), second read D_0015EE8
 *   Unblock: find a wording where the +0xAA35 access is not CSE-able with +0xAB75 (possibly different struct/array
 */
#include "common.h"
extern int func_001E9730();
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001F9D10(void *, void *);
extern void func_L00_00261B00(void *, int, int, int, int);
extern char D_L00_001E9280[];
extern int D_L00_001BA860[];
extern char D_L00_001BA960[];
extern unsigned char D_L00_0015FD48[];
extern char D_L00_001BB5C0[];
extern char D_L00_001E92A8[];
extern short D_L00_0016007C;
extern int D_L00_0016007C_m __asm__("D_L00_0016007C") NOT_SDA;
extern short D_0015EE84;
extern int D_0015EE84_m __asm__("D_0015EE84") NOT_SDA;
extern unsigned char D_0014171B[] NOT_SDA;
extern unsigned char D_0013E633[] NOT_SDA;

/* Sets the death bits of a moby's spawn slot and emits its sound with a volume derived from its values. */
void func_L00_002584A8(char *m, int flags, int arg3) {
    float f = *(short *)(m + 0xB4);
    int v, y, g, r;
    unsigned char *base;
    unsigned char *q;
    char *p;
    func_001E9730(D_L00_001E9280, *(short *)(m + 0xA6), *(short *)(m + 0xB2), *(short *)(m + 0xB4), *(short *)(m + 0xB6));
    if (*(unsigned char *)(m + 0xB1) != 0xFE && *(short *)(m + 0xB2) >= 0) {
        *(int *)(D_0014171B + 0xAB75 + (*(int *)&D_0015EE84 << 8) + (*(short *)(m + 0xB2) >> 5) * 4) |= 1 << (*(unsigned short *)(m + 0xB2) & 0x1F);
        D_L00_001BA860[*(short *)(m + 0xB2) >> 5] |= 1 << (*(unsigned short *)(m + 0xB2) & 0x1F);
        p = D_L00_001BA960 + *(short *)(m + 0xB2);
        p[0x454] = *(unsigned char *)(m + 0xB0) + 2;
        if (*(unsigned char *)(m + 0xB0) == 0xFF
            || (D_L00_0015FD48[*(unsigned char *)(m + 0xB0)] != 0xFF
                && D_0014171B[0xAA35 + (*(int *)&D_0015EE84 << 4) + *(unsigned char *)(m + 0xB0)] == 0xFF)) {
            p = D_L00_001BB5C0 + *(short *)(m + 0xB2);
            p[0x454] = *(unsigned char *)(m + 0xB0) + 2;
        }
        if (*(char *)(m + 0xB1) >= 0) {
            int s = *(short *)(m + 0xB6);
            int d = s - *(short *)(D_0014171B + 0xBF75 + *(unsigned char *)(m + 0xB1) * 4 + (D_0015EE84_m << 8) + 2);
            if ((s + 1) / 2 >= d) d = 0;
            if (d != 0) f = d;
            if (f < 1.0f) f = 1.0f;
            func_001E9730(D_L00_001E92A8, func_001FA898_r(f));
        }
        base = D_0013E633 + 0xE1D;
        v = 1;
        if (flags & 0x100) v = 5;
        if (base[0x20B2] != 0 || (flags & 0x200)) v |= 2;
        q = D_0013E633 + 0xE1D;
        if (*(int *)(q + 0x2084) == 0x10) {
            if (func_001F9D10(q + 0x80, m + 0x10) < 4.0f) v |= 2;
            y = D_L00_0016007C_m;
        } else {
            y = *(int *)&D_L00_0016007C;
        }
        if (flags & 0x400) v |= 0x10;
        if (y < 100 || (flags & 0x800)) v |= 8;
        if (y < 0x32) v |= 2;
        if (f < 0.0f) f = 0.0f;
        if (f > 0.0f) {
            g = 0;
            if (f >= 8.0f) g = func_001FA898_r(f * 0.25f);
            else if (f >= 2.0f) g = 1;
            r = func_001FA898_r(f);
            func_L00_00261B00(m, r - g, r + g, v, arg3);
        }
    }
}
