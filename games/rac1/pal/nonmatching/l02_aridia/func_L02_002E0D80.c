/* NON_MATCHING func_L02_002E0D80 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: BYTES 19/652 (97.1% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L02_002E0D80 (652 B) - staged c7 at 19 B
 *   Logic complete. Left: the case-0 table lookup D_0014C150[m->B0 + lvl*16] == 0xFF schedules
 *   lui/addiu %lo(tbl) and li 0xFF before the D_0015EE84 load; retail loads D_0015EE84 first.
 *   Tried: [][16] form, index orders, << 4, D_0014171B + 0xAA35 (NOT_SDA), level temp, int store for d[3],
 *   pointer temp, inverted if/else (much worse). None move it.
 */
extern int D_L02_0015F6A8 MACRO_ADDR;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern unsigned char D_0014C150[] NOT_SDA;
extern void func_L00_001FFED8(void *, int, float);
extern void func_001FA588(void *, void *, void *);
extern float func_L00_0025CE58(float *, float *, float, float, float, float);
extern void func_001FA5C8(void *, void *, void *, float);
extern void func_001FA648(void *, void *);

/* Spinning-platform update: captures its start rotation, then spins it while active. */
void func_L02_002E0D80(char *m) {
    float *d = *(float **)(m + 0x78);
    float rx[4];
    float ry[4];
    float rz[4];
    float t[4];
    float t2[4];
    if (D_L02_0015F6A8 == 2 && ((unsigned char *)m)[0xBC] == 0) {
        m[0x31] = 0;
        *(unsigned short *)(m + 0x34) |= 1;
    } else {
        m[0x31] = 1;
        *(unsigned short *)(m + 0x34) &= ~1;
    }
    switch (((unsigned char *)m)[0x20]) {
    case 0:
        qcopy(d, m + 0x40);
        d[3] = 0;
        if (D_0014C150[((unsigned char *)m)[0xB0] + D_0015EE84_m * 16] == 0xFF) {
            *(float *)(m + 0x40) = d[4] * 0.017453292f;
            *(float *)(m + 0x44) = d[5] * 0.017453292f;
            *(float *)(m + 0x48) = d[6] * 0.017453292f;
            m[0x20] = 3;
        } else {
            m[0x20] = 1;
            *(unsigned short *)(m + 0x34) |= 0x100;
            func_L00_001FFED8(rx, 0, d[4] * 0.017453292f);
            func_L00_001FFED8(ry, 1, d[5] * 0.017453292f);
            func_L00_001FFED8(rz, 2, d[6] * 0.017453292f);
            func_001FA588(t, rx, ry);
            func_001FA588(d + 4, t, rz);
            func_L00_001FFED8(rx, 0, *(float *)(m + 0x40));
            func_L00_001FFED8(ry, 1, *(float *)(m + 0x44));
            func_L00_001FFED8(rz, 2, *(float *)(m + 0x48));
            func_001FA588(t2, rx, ry);
            func_001FA588(d, t2, rz);
        }
        break;
    case 1:
        if (((unsigned char *)m)[0xBC] != 0) {
            m[0x20] = 2;
            d[8] = 0;
            d[9] = 0;
        }
        break;
    case 2:
        func_L00_0025CE58(d + 9, d + 8, 1.0f, D_0015EE70 * 0.5f, D_0015EE70 * 0.5f, D_0015EE6C);
        func_001FA5C8(rx, d, d + 4, d[9]);
        func_001FA648(rx, m + 0xC0);
        break;
    }
}
