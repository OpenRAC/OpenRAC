/* NON_MATCHING func_L11_00311F98 -- src/overlays/shared/vendor_002C99E0.c
 * Best so far: SIZE ours 532 / retail 540, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Writes a GIF/DMA packet (tag + 3 header quads + n packed points) at the display-list pointer D_L11_00161240 an
 *   p4.c: 532 vs 540 bytes; pointer-bump via local p (p=D; D=p+0x10; write p+0x10) matched the sh/sq addressing. L
 *   Runs 1-2 lost to a type clash (D_0013E15A is unsigned char[] in the file) and a mac sed error.
 */
typedef int u128_311F98 __attribute__((mode(TI)));
extern float func_001FA888(int);
extern int func_001FA898(float);
extern char *D_L11_00161240 MACRO_ADDR;
extern char D_L11_00160960[];
extern char D_L11_00160970[];
extern unsigned char D_0013E15A[];

// Emits a GIF packet of n scaled points (pts is pairs of shorts) into the display list.
void func_L11_00311F98(short *pts, int n, int c, long w, float ox, float oy, float scale) {
    char *g;
    long *out;
    char *p;
    short *q;
    int i;
    *(int *)D_L11_00161240 = ((n + 1) / 2 + 3) | 0x10000000;
    *(int *)(D_L11_00161240 + 4) = 0;
    *(int *)(D_L11_00161240 + 8) = 0;
    *(int *)(D_L11_00161240 + 0xC) = ((n + 1) / 2 + 3) | 0x50000000;
    p = D_L11_00161240;
    D_L11_00161240 = p + 0x10;
    qcopy(p + 0x10, D_L11_00160960);
    *(short *)(p + 0x10) = 0x8001;
    p = D_L11_00161240;
    D_L11_00161240 = p + 0x10;
    *(long *)(p + 0x10) = 0x144;
    *(long *)(p + 0x18) = w;
    p = D_L11_00161240;
    D_L11_00161240 = p + 0x10;
    qcopy(p + 0x10, D_L11_00160970);
    *(short *)(p + 0x10) = n - 0x8000;
    p = D_L11_00161240;
    D_L11_00161240 = p + 0x10;
    out = (long *)(p + 0x10);
    if (n > 0) {
    g = (char *)D_0013E15A + 0x4A6;
    i = n;
    q = pts;
    do {
        int x = func_001FA898((func_001FA888(q[0]) * scale + ox) * 16.0f);
        int y = func_001FA898((func_001FA888(q[1]) * scale + oy) * 16.0f);
        q += 2;
        *out++ = (long)(x + *(int *)(g + 0x10) - 8) | ((long)(y + *(int *)(g + 0x14) - 8) << 16) | ((long)c << 32);
    } while (--i != 0);
    }
    D_L11_00161240 += ((n + 1) / 2) << 4;
}
