/* NON_MATCHING func_L13_002CFF90 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: BYTES 92/776 (88.1% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef int Q_2cff90 __attribute__((mode(TI)));
extern char D_L13_001F4C70[];
extern char D_L13_001F4C98[];
extern int func_00120778(float);
extern int func_001E9730_d(void *, ...) __asm__("func_001E9730");
extern void func_L00_002584A8(void *, int, int);
extern void func_L00_00250800(void *, int, void *);
extern void func_L00_0025F4A8_alt(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int) __asm__("func_L00_0025F4A8");
extern void func_L00_0028EBF0(int);

/* Enemy hit reaction (debug build prints the hits): loses health from attacks landing from above; at zero it
 * bursts into debris and dies. */
void func_L13_002CFF90(char *m, char *d, float *hp) {
    if (((unsigned char *)m)[0x20] != 2) {
        int cnt;
        float u[4];
        float v[4];
        char *t = func_L00_0025B478(m, 0x330000, 0);
        if (t != 0) {
            int a, b, c;
            char *fmt = D_L13_001F4C70;
            int id = *(short *)(m + 0xA6);
            func_001E9730_d(fmt, id, func_00120778(*(float *)(t + 0x2C)));
            fmt = D_L13_001F4C98;
            a = func_00120778(*(float *)(t + 0));
            b = func_00120778(*(float *)(t + 4));
            c = func_00120778(*(float *)(t + 8));
            func_001E9730_d(fmt, *(int *)(t + 0x38), a, b, c);
        }
        switch (func_L00_0025B4D0(m, t, hp, 0, &cnt, 0, 0, 4)) {
        case 1:
        case 2:
            *hp = 0;
            break;
        case 0:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            break;
        }
        if (cnt >= 2) {
            if (t == 0) {
                ((unsigned char *)m)[0xA4] = 0xFF;
                goto end;
            }
            if (*(char **)(t + 0x20) != 0 && *(float *)(m + 0x18) + 8.7f < *(float *)(*(char **)(t + 0x20) + 0x18)) {
                if (*hp <= *(float *)(t + 0x2C)) {
                    *(Q_2cff90 *)v = 0;
                    *hp = 0;
                    v[2] = D_0015EE6C * 8.0f;
                    *(unsigned short *)(m + 0x34) &= 0xEFFF;
                    *(Q_2cff90 *)u = *(Q_2cff90 *)v;
                    func_L00_002584A8(m, 0, -1);
                    d[0x67] = 0x78;
                    func_L00_0025E4B0(m, (short *)(d + 0x60));
                    func_L00_00250800(m, 0, v);
                    func_L00_0025F4A8_alt(m, u, v, 0.0f, 0.0f, 0xA, 3, 0x10, 4.0f, 2.0f, 0.0f, 1.0f, 1, 20.0f, 1, 1, -1, 0);
                    if (((unsigned char *)m)[0x53] != 1) func_00213DE0(m, 1, 0, 0);
                    if (*(int *)(d + 0x70) != -1) {
                        char *e = D_0013E633 + 0x1D + *(int *)(d + 0x70) * 0x70;
                        if (*(char **)(e + 0x88) == m && ((unsigned char *)e)[0x74] != 0) {
                            func_L00_0028EBF0(*(int *)(d + 0x70));
                        }
                    }
                    *(int *)(d + 0x70) = -1;
                    m[0x20] = 2;
                } else {
                    *hp = *hp - *(float *)(t + 0x2C);
                    ((unsigned char *)d)[0x67] = 0xFA;
                    *(short *)(d + 0x26) = func_001F9850(0x3C);
                    func_L00_0025E4B0(m, (short *)(d + 0x60));
                }
            }
        }
        ((unsigned char *)m)[0xA4] = 0xFF;
    }
end:
    func_L00_0025E590(m, d + 0x60);
}
