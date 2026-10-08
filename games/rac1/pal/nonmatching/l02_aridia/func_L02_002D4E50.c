/* NON_MATCHING func_L02_002D4E50 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: BYTES 8/580 (98.6% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns d particles along vb-va (func_L00_0026DEA0 with random color/speed). p5.c is 21/580 bytes off: only the
 *   col = c | c<<8 | c<<16 (retail: sll 8, sll 16, or v1=v1|v0, or c) and the order of the f13/f14/a1/a2 argument 
 *   Try c | (c << 16) | (c << 8) and moving the s[2] store; needs another pass.
 *   q27 s08: best p9.c = 16/580 (col = c | ((c << 16) | (c << 8)) fixes the shift order). Left: order of f14/a1 an
 */
typedef int u128b __attribute__((mode(TI)));
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_001FA888(int);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_00258BC8(int, int);
extern float func_002140F8(float, float);
extern float func_L00_00258C80(float lo, float hi);
extern char * func_L00_0026DEA0__s(void *, int, float, float, void *, int, float, float) __asm__("func_L00_0026DEA0");
extern int func_002140B0(int);
extern short D_L02_00161A64;
extern short D_L02_00161A6C;
extern short D_L02_00161A70;

// Spawns a streak of d particles interpolated from va to vb.
void func_L02_002D4E50(void *moby, void *va_, void *vb_, int d, float fa, float fb) {
    float *va = va_;
    float *vb = vb_;
    float out[4];
    float tmp[4];
    float s[4];
    int i;
    float fd = d;
    float diff;
    float step;
    func_001F9BF0(out, vb, va);
    func_001F9C30(out, out, 1.0f / fd);
    diff = fb - fa;
    *(u128b *)s = 0;
    s[2] = 0.01f;
    s[3] = 1.0f;
    step = diff / fd;
    for (i = 0; i < d; i++) {
        int a, c, col, r;
        float sp;
        char *p;
        func_001F9C30(tmp, out, func_001FA888(i));
        func_001F9BD8(tmp, va, tmp);
        a = func_L00_00258BC8(0x40, 0x70);
        c = func_L00_00258BC8(0x40, 0x7F);
        col = c | ((c << 16) | (c << 8));
        sp = func_002140F8(1.0f, 1.02f);
        r = func_L00_00258BC8(-2, 2);
        s[0] = func_L00_00258C80(0.0f, 0.0025f);
        s[1] = func_L00_00258C80(0.0f, 0.0025f);
        s[2] = func_002140F8(*(float *)&D_L02_00161A64 * 0.1f, *(float *)&D_L02_00161A64);
        p = func_L00_0026DEA0__s(tmp, r, *(float *)&D_L02_00161A6C, 1.0f, s, (a << 24) | col, sp, step + fa * 210000.0f);
        if (p != 0) {
            char *q = p + 0x20;
            if (func_002140B0(2) != 0) {
                p[3] = 0x44;
            }
            *(unsigned short *)(p + 0xA) = *(unsigned short *)&D_L02_00161A70;
            *(int *)(q + 4) = 2;
            q[0xA] = a;
            q[0xB] = *(unsigned char *)&D_L02_00161A70;
        }
    }
}
