/* NON_MATCHING func_L07_0031B318 -- src/overlays/l07_umbris/vendor_00313D28.c
 * Best so far: BYTES 6/772 (99.2% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern float D_0015EE6C_b __asm__("D_0015EE6C") MACRO_ADDR;
extern char *func_L00_0025B478_b(void *, int, int) __asm__("func_L00_0025B478");
extern void func_001F9EC0(void *, void *, void *);
extern void func_L00_00258DB0(float *, float, float);
extern int func_L01_002F9908_x(float, void *, void *, float, float, float, unsigned int, int, int) __asm__("func_L01_002F9908");
extern void func_L00_0025F4A8_b(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int) __asm__("func_L00_0025F4A8");

/* Breakable crate: when smashed by the charged attack of one of the two hammer mobys, scatters d->0
 * debris pieces over its (rotated) extent, explodes and is deleted. */
void func_L07_0031B318(char *m) {
    float ext[4];
    float p[4];
    float v[4];
    char *d = *(char **)(m + 0x78);
    char *h = func_L00_0025B478_b(m, 0x30000, 0);
    if (h != 0) {
        char *a = *(char **)(h + 0x20);
        if (a != 0
            && ((*(short *)(a + 0xA6) == 0x415 && ((unsigned char *)a)[0x20] == 8)
                || (*(short *)(a + 0xA6) == 0x452 && ((unsigned char *)a)[0x20] == 9))) {
            if (*(float *)(h + 0x2C) == 1.000123f) {
                int i;
                ext[0] = *(float *)(d + 4);
                ext[1] = *(float *)(d + 8);
                ext[2] = *(float *)(d + 0xC);
                func_001F9EC0(ext, ext, m + 0xC0);
                for (i = 0; i < *(int *)d; i++) {
                    p[0] = func_002140F8(-ext[0], ext[0]) * 0.5f;
                    p[1] = func_002140F8(-ext[1], ext[1]) * 0.5f;
                    p[2] = func_002140F8(1.5f, ext[2]);
                    func_001F9BD8(p, p, m + 0x10);
                    func_L00_00258DB0(v, D_0015EE6C_b * 5.0f, D_0015EE6C_b * 10.0f);
                    if (v[2] < 0.0f) v[2] = -v[2];
                    if (v[2] < D_0015EE6C_b * 2.0f) v[2] += D_0015EE6C_b * 2.0f;
                    {
                        unsigned int k = i % 3 + 0x2B8;
                        float sc = func_002140F8(0.41f, 0.5f);
                        func_L01_002F9908_x(sc, p, v, 1.0f, 1.0f, 0.75f, k, func_L00_00258BC8(0x32, 0x50), 0);
                    }
                }
                func_L00_0025F4A8_b(m, h + 0x10, h, 3.0f, 1.0f, 10, 3, 16, 4.0f, 2.0f, 9.0f, 1.0f, 0, 15.0f, 1, 1, -1, 0);
                func_0020D678(m);
                return;
            }
        }
    }
    ((unsigned char *)m)[0xA4] = 0xFF;
}
