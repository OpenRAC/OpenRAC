/* NON_MATCHING func_L12_0030D3D0 -- src/overlays/l12_hoven/vendor_002EDAA0.c
 * Best so far: SIZE ours 736 / retail 748, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern short D_L12_00162158, D_L12_00162160, D_L12_00162170;
extern float D_L12_0016215C MACRO_ADDR;
extern float D_L12_00162164[];
extern float D_L12_00162178[];
extern float D_L12_0016217C[];
extern float D_L12_00162188_f __asm__("D_L12_00162188") MACRO_ADDR;
extern float D_L12_0016218C MACRO_ADDR;
extern float D_0015EE7C MACRO_ADDR;
extern char D_L12_00208DA0[];
extern char D_L12_00205D40[];
extern char D_L12_001FBFD0_s[] __asm__("D_L12_001FBFD0");
extern void func_L08_002F2428_s(void *, int, void *, int, float, float, float, float) __asm__("func_L08_002F2428");
extern void func_001F49B0(void *, void *);
extern void func_L12_0030D248(char *m);

/* Hoven water: builds the water textures once, then scrolls the three texture layers' UV offsets
 * (wrapped to -1..1) and queues the water draw. */
void func_L12_0030D3D0(char *m) {
    int i;
    switch (((unsigned char *)m)[0x20]) {
    case 0:
        if (*(int *)&D_L12_00162170 == 0) {
            func_L08_002F2428_s(D_L12_00208DA0, 5, D_L12_00205D40, 1, 0.1f, 0.5f, 0.4f, 0.5f);
            func_L08_002F2428_s(D_L12_00205D18, 10, D_L12_001FBFD0_s, 1, 0.1f, 0.6f, 0.4f, 0.5f);
            *(int *)&D_L12_00162170 = 1;
        }
        m[0x20] = 1;
        *(short *)(m + 0x32) = 0xFF;
        ((unsigned char *)m)[0x30] = 0xFF;
        for (i = 0; i < 2; i++) {
            D_L12_00162178[i * 2] = 0.0f;
            D_L12_0016217C[i * 2] = 0.0f;
        }
        break;
    case 1:
        for (i = 0; i < 2; i++) {
            D_L12_00162178[i * 2] = D_L12_00162178[i * 2] + ((float *)&D_L12_00162160)[i * 2] * D_0015EE7C;
            if (1.0f < D_L12_00162178[i * 2]) D_L12_00162178[i * 2] -= 1.0f;
            if (D_L12_00162178[i * 2] < -1.0f) D_L12_00162178[i * 2] += 1.0f;
            D_L12_0016217C[i * 2] = D_L12_0016217C[i * 2] + D_L12_00162164[i * 2] * D_0015EE7C;
            if (1.0f < D_L12_0016217C[i * 2]) D_L12_0016217C[i * 2] -= 1.0f;
            if (D_L12_0016217C[i * 2] < -1.0f) D_L12_0016217C[i * 2] += 1.0f;
        }
        D_L12_00162188_f = D_L12_00162188_f + *(float *)&D_L12_00162158 * D_0015EE7C;
        if (1.0f < D_L12_00162188_f) D_L12_00162188_f -= 1.0f;
        if (D_L12_00162188_f < -1.0f) D_L12_00162188_f += 1.0f;
        D_L12_0016218C = D_L12_0016218C + D_L12_0016215C * D_0015EE7C;
        if (1.0f < D_L12_0016218C) D_L12_0016218C -= 1.0f;
        if (D_L12_0016218C < -1.0f) D_L12_0016218C += 1.0f;
        func_001F49B0(func_L12_0030D248, m);
        break;
    }
}
