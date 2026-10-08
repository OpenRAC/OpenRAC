/* NON_MATCHING func_L05_003108D0 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: BYTES 25/444 (94.4% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Steers two angles of a moby toward a target and calls func_L00_0025CE58 twice. Best p1.c (32 bytes differ).
 *   Structure matches; only float register numbers (a vs y swapped f22/f23) and one mul/arg-setup order differ. Op
 *   x03 (q30): p5 (x,y declaration order), p6 (dot*a operand order), p7 (x=0;y=x): same 32-byte diff. Retail: a=$f
 */
extern char D_0013E633[];
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001F9C78(void *a, void *b);
extern float func_001FA748(float, float);
extern float func_001F9FA8(float);
extern float func_L00_0025CE58__s(float *p, float a, float *v, float b, float c, float d) __asm__("func_L00_0025CE58");
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L05_00161E98;
extern short D_L05_00161E9C;
extern short D_L05_00161EA0;
extern short D_L05_00161EA4;

// Steers a moby's two angles toward a target and updates its velocity vectors.
void func_L05_003108D0(char *moby, float a, float b) {
    char *g = D_0013E633 + 0xE1D;
    char *data = *(char **)(moby + 0x78);
    float tmp[4];
    float y, x;
    if (*(char **)(g + 0x2FC) == moby && *(short *)(g + 0x30E) == 0) {
        func_001F9BF0(tmp, g + 0x80, moby + 0x10);
        y = a * func_001F9C78(tmp, moby + 0xC0);
        x = -a * func_001F9C78(tmp, moby + 0xD0);
    } else {
        y = 0.0f;
        x = y;
    }
    *(float *)(data + 0x98) = func_001FA748(*(float *)(data + 0x98), *(float *)(data + 0xA0));
    *(float *)(data + 0x9C) = func_001FA748(*(float *)(data + 0x9C), *(float *)(data + 0xA4));
    x = func_001FA748(x, func_001F9FA8(*(float *)(data + 0x98)) * a * *(float *)&D_L05_00161E98);
    y = func_001FA748(y, func_001F9FA8(*(float *)(data + 0x9C)) * a * *(float *)&D_L05_00161E98);
    func_L00_0025CE58__s((float *)(moby + 0x40), x, (float *)(data + 0x90), *(float *)&D_L05_00161E9C * b * 0.017453292f * D_0015EE70, *(float *)&D_L05_00161EA0 * b * 0.017453292f * D_0015EE70, *(float *)&D_L05_00161EA4 * 0.017453292f * D_0015EE6C);
    func_L00_0025CE58__s((float *)(moby + 0x44), y, (float *)(data + 0x94), *(float *)&D_L05_00161E9C * b * 0.017453292f * D_0015EE70, *(float *)&D_L05_00161EA0 * b * 0.017453292f * D_0015EE70, *(float *)&D_L05_00161EA4 * 0.017453292f * D_0015EE6C);
}
