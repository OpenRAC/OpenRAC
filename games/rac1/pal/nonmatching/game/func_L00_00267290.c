/* Veldin level code: one step of the stream/event slot machine for the request b against actor a
 * (retail func_L00_00267290, 0x388 bytes). Gates on the camera block at D_0013E633 + 0xE1D (0x2084 mode,
 * 0x22A8 enable). Returns 1 when it consumed the request, 0 otherwise. Tables: 28-byte records at b+0x3C
 * indexed by b+0x36; D_L00_00179200 holds the last request's a/b and a counter. */
extern char D_0013E633[];
extern char D_0013A5E0[];
extern char D_0013D605[];
extern char D_0013D355[];
extern char D_L00_00179200[];
extern char D_L00_001C43B0[];
extern s32 D_L00_0015F6A8 MACRO_ADDR;
extern s32 D_L00_0015F6B0 MACRO_ADDR;
extern s32 D_0015EE98 MACRO_ADDR;
extern s32 D_L00_001601F0 MACRO_ADDR;
extern s32 func_L00_002676E8(void *p, u8 *out_p);
extern s32 func_L00_00267618_290(void *) __asm__("func_L00_00267618");
extern float func_001F9D10(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern void func_L00_00217AE8(void *, void *, int);
extern int func_L00_002367A8(int, int);
extern int func_001FFB38_290(int, int, void *, void *, void *, void *, int) __asm__("func_001FFB38");
extern int func_001F9850(int);
extern void func_001FFCB0(int);
extern void func_L00_0029A7D0(int);
extern void func_L00_00299B68(int);
extern void func_L00_00237B20(void);
extern void func_L00_00237B70(void);
extern void func_L00_00237B90(void);
extern void func_L00_0023A658(void);
extern void func_L00_0023A690(void);
extern void func_L00_0023A788(void);

s32 func_L00_00267290(void *a0, void *b0) {
    char *a = (char *)a0;
    char *b = (char *)b0;
    char *H;
    char *g2;
    char *E;
    char *E2;
    char *p;
    float f0, f1, f12, f13;
    s32 r, x, h8, h4, flags, ha, res, t;

    H = D_0013E633 + 0xE1D;
    if (*(s32 *)(H + 0x2084) == 0x1D)
        return 0;
    if (*(s32 *)(H + 0x22A8) == 0)
        return 0;
    if (*(u8 *)(b + 0x9) == 0)
        func_L00_002676E8(a, b);
    if (D_L00_0015F6A8 != 0)
        return 0;
    if (b == 0)
        return 0;
    if (*(s16 *)(b + 0x36) == -1)
        return 0;
    if (*(u8 *)(b + 0x8) == 0xFF)
        return 0;

    f0 = func_001F9D10(a + 0x10, H + 0x80);
    f1 = *(float *)(b + 0xC);
    f1 = f1 + f1;
    if (f1 < f0)
        return 0;

    if (*(u8 *)(b + 0x8) == 0) {
        f1 = *(float *)(H + 0x80);
        f12 = *(float *)(a + 0x10);
        f0 = *(float *)(H + 0x84);
        f13 = *(float *)(a + 0x14);
        f12 = f1 - f12;
        f13 = f0 - f13;
        f0 = func_L00_001FF860(f12, f13);
        f13 = *(float *)(a + 0x48);
        f0 = func_001FA850(f0, f13);
        f1 = *(float *)(b + 0xC);
        if (f1 < f0)
            return 0;

        f1 = *(float *)(a + 0x10);
        f12 = *(float *)(H + 0x80);
        f0 = *(float *)(a + 0x14);
        f13 = *(float *)(H + 0x84);
        f12 = f1 - f12;
        f13 = f0 - f13;
        f0 = func_L00_001FF860(f12, f13);
        f13 = *(float *)(H + 0x98);
        f0 = func_001FA850(f0, f13);
        f1 = 1.57f;
        if (f1 < f0)
            return 0;
    }

    func_L00_00217AE8(a, b, 0);

    g2 = D_L00_00179200;
    if (D_L00_0015F6B0 < *(s32 *)(g2 + 0x10))
        return 0;

    if (*(u8 *)(b + 0x8) == 0 && (*(s32 *)(D_0013A5E0 + 0x2604) & 0x10) == 0) {
        r = func_L00_002367A8(D_L00_001601F0, 0xA);
        if (r != 0)
            D_L00_001601F0 = func_001FFB38_290(0xC, 0, (void *)func_L00_00237B20, (void *)func_L00_00237B70,
                                               (void *)func_L00_00237B90, 0, 0);
    } else {
        r = func_L00_00267618_290(a);
        p = D_0013D605 + 0xB3 + 0xC + (r << 4);
        if (*(s32 *)p == 0)
            *(s32 *)p = 1;
        *(s32 *)(g2 + 0xC) = (s32)b;
        *(s32 *)(g2 + 0x8) = (s32)a;
        E = *(char **)(b + 0x3C) + (*(s16 *)(b + 0x36)) * 0x1C;
        if (*(u16 *)(E + 0x10) & 0x8) {
            h8 = *(s16 *)(E + 0x8);
            if (h8 == 1) {
                ha = *(s16 *)(E + 0xA);
                x = D_0015EE98 - *(s32 *)(D_L00_001C43B0 + ha * 0x18);
                D_0015EE98 = x;
            } else if (h8 == 4) {
                *(u8 *)(D_0013D355 + 0x13B + *(s16 *)(E + 0xA)) = 0;
            }
        }
        h4 = *(s16 *)(E + 0x4);
        if (h4 == -1) {
            func_L00_00217AE8(a, b, 1);
            return 1;
        }
        *(s32 *)(g2 + 0xC) = (s32)b;
        *(s32 *)(g2 + 0x8) = (s32)a;
        flags = *(u16 *)(E + 0x4);
        if (flags & 0x4000) {
            func_L00_0029A7D0((s16)(flags ^ 0x4000));
            return 1;
        }
        func_001FFCB0(D_L00_001601F0);
        D_L00_001601F0 = -1;
        func_L00_00299B68(*(s16 *)(E + 0x4));
        return 1;
    }

    /* tail: the record at b's table entry; only reached when the request was not consumed above */
    E2 = *(char **)(b + 0x3C) + (*(s16 *)(b + 0x36)) * 0x1C;
    if (*(s16 *)(E2 + 0x8) != 1) {
        if (*(s16 *)(E2 + 0x8) != 6)
            return 0;
    }
    res = func_001FFB38_290(2, 0x754E, (void *)func_L00_0023A658, (void *)func_L00_0023A690,
                            (void *)func_L00_0023A788, (void *)&D_0015EE98, 0x98967F);
    t = func_001F9850(0x3C);
    func_L00_002367A8(res, t);
    return 0;
}
