/* NON_MATCHING func_L16_002E7670 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: SIZE ours 548 / retail 544, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   The exploratory p0.c had an extra call absent from retail. p1 removes it and uses `qcopy`; p2 casts the four l
 */
extern char *D_L16_001B0C30[];
extern char D_0013E633[];
extern float D_0015EE6C;
extern char D_L16_0015EE70 MACRO_ADDR;
extern char D_L16_00161F14 MACRO_ADDR;
extern char D_L16_00161F18 MACRO_ADDR;
extern char D_L16_00161F1C MACRO_ADDR;
extern char D_L16_00161F20 MACRO_ADDR;
extern char *func_L00_0025B478(void *, int, int);
extern void func_L00_0025BBA0(void *, void *, void *, void *);
extern void func_L00_0025D5B0(void *, void *, int, int, int, float);
extern void func_L00_0025E4B0(void *, void *);
extern void func_L00_0025E590(void *, void *);
extern int func_L00_00260FB0(void *, void *, float, int, int, void *, int);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
extern void func_L16_002E5D68(void *);

void func_L16_002E7670(void *m_v) {
    unsigned char *m = m_v;
    char *d = *(char **)(m + 0x78);
    char *hit;
    char *path;
    float v[8];
    if (m[0x20] == 0) return;
    hit = func_L00_0025B478(m, 0x330000, 0);
    if (hit) {
        float scale = *(float *)&D_L16_0015EE70;
        float base = D_0015EE6C;
        float f4 = *(float *)&D_L16_00161F14 * scale;
        float f3 = *(float *)&D_L16_00161F18 * scale;
        float f2 = *(float *)&D_L16_00161F20 * base;
        float f1 = *(float *)&D_L16_00161F1C * base;
        *(unsigned char *)(d + 0x15D) = 0;
        *(float *)(d + 0x16C) = D_0015EE6C + D_0015EE6C;
        *(float *)(d + 0x130) = f4;
        *(float *)(d + 0x134) = f3;
        *(float *)(d + 0x138) = f2;
        *(float *)(d + 0x13C) = f1;
        *(int *)(d + 0x140) = 0x200;
        *(int *)(d + 0x144) = 0x29;
        *(float *)(d + 0x148) = 0.5f;
        *(unsigned short *)(m + 0x34) &= ~0x1000;
        qcopy(v, hit + 0x10);
        func_L00_0025BBA0(v, v + 4, d + 0x138, d + 0x13C);
        func_L00_0025D5B0(m, d + 0x120, 5, 1, 0, v[4]);
        *(float *)(d + 0x170) = 8.0f;
        *(float *)(d + 0x174) = 16.0f;
        m[0x20] = 8;
        *(int *)(m + 0x94) = 0;
        *(unsigned char *)(d + 0x117) = 0x78;
        func_L00_0025E4B0(m, d + 0x110);
    }
    m[0xA4] = 0xFF;
    func_L00_0025E590(m, d + 0x110);
    path = D_L16_001B0C30[*(int *)(d + 0x1F0)];
    if (func_L00_00260FB0(m, d + 0x180, 16.0f, 0, 0, (void *)(path + 0x10), *(int *)path) != 2) {
        if (func_001F9D48(d + 0x1D0, d + 0x180) > 16.0f ||
            func_001F9B88(*(float *)(m + 0x18) - *(float *)(d + 0x188)) > 3.0f) {
            *(int *)(d + 0x1C4) = 2;
        }
    }
    if (*(int *)(d + 0x1C0) == 0) {
        char *base = D_0013E633 + 0xE1D;
        *(int *)(d + 0x1C0) = *(int *)(base + 0x2080);
        qcopy(d + 0x180, base + 0x80);
    }
}
