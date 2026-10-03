/* NON_MATCHING func_L16_002D0328 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: BYTES 30/568 (94.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Reconstructed the list walk, color setup, and vector calls in plain C. `p9.c` has exact 568 byte size and diff
 */
extern unsigned long func_001F4868(int);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9FA8(float);
extern int func_001FA8A8_0328(int, int, float) __asm__("func_001FA8A8");
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L16_001D3400[];
extern char D_L16_00167240[];
extern char D_0013E633[];
extern char D_L16_00161AA0 MACRO_ADDR;
extern char D_L16_00161AA4 MACRO_ADDR;

void func_L16_002D0328(unsigned char *m) {
    struct {
        float result[16];
        int colors[4];
        float seed[8];
        unsigned long cfg[4];
        float a[4], b[4], c[4], pos[4];
    } x;
    short *p;
    char *other;
    int k, color;
    float *out;
    char *target;
    x.cfg[1] = func_001F4868(11);
    x.cfg[3] = 0x8000000048UL;
    x.cfg[2] = 0xFF9000000260UL;
    x.cfg[0] = 5;
    x.seed[0] = 0.0f;
    x.seed[1] = 0.0f;
    x.seed[2] = 0.0f;
    x.seed[3] = 1.0f;
    x.seed[4] = 1.0f;
    x.seed[5] = 0.0f;
    x.seed[6] = 1.0f;
    x.seed[7] = 1.0f;
    out = x.result;
    target = D_L16_001D3400;
    for (k = 3; k >= 0; k--) {
        func_001F9C30(out, target, 0.2f);
        out += 4;
        target += 16;
    }
    p = (short *)D_L16_001ABFC0[m[0x21]];
    do {
        other = D_L16_00160098 + ((*p & 0x7FFF) << 8);
        color = func_001FA8A8_0328(*(int *)&D_L16_00161AA0, *(int *)&D_L16_00161AA4,
                              (func_001F9FA8(*(float *)(*(char **)(other + 0x78) + 0x11C)) + 1.0f) * 0.5f);
        x.colors[3] = color;
        x.colors[2] = color;
        x.colors[1] = color;
        x.colors[0] = color;
        if (!other) continue;
        switch (*(short *)(other + 0xA6)) {
        case 0x21D: break;
        default: continue;
        }
        if ((unsigned char)other[0x20] != 0xFE && (unsigned char)other[0x20] != 0xFD) {
            qcopy(x.pos, other + 0x10);
            x.pos[2] += 0.2f;
            x.pos[3] = 1.0f;
            func_001F9BF0(x.a, D_L16_00167240, x.pos);
            func_L00_001FF4B0(x.a, x.a, 1.0f);
            func_001F9CA0(x.b, x.a, D_0013E633 + 0x10AD);
            func_L00_001FF4B0(x.b, x.b, -1.0f);
            func_001F9CA0(x.c, x.b, x.a);
            func_L00_001FD1D8(&x, x.a, 0);
        }
    } while (*p++ >= 0);
}
