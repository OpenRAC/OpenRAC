/* NON_MATCHING func_L00_002EB938 -- src/overlays/shared/vendor_002EB0D8.c
 * Best so far: BYTES 3/788 (99.6% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   InitCamera_4 update (a=moby, b=other camera moby via $a1): mode-transition counter in a per-entry struct, with
 *   Left: one delay slot after the second `bc1t` (f20 == 0 check) is `daddu $a0,$s0,$zero` in retail but `nop` in 
 */
typedef int u128_2EB938 __attribute__((mode(TI)));
extern int func_L00_001EE530(void *, float);
extern int func_001F9850(int);
extern float func_001F9F90(float x);
extern float func_001F9FA8(float);
extern void func_L00_002E9DC8(void *a, float x, float y);
extern float func_001F9C78(void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern float func_001F9FC0(float);
extern void func_L00_001ED600(void);
extern int func_001F9908(int *);
extern char *D_L00_0015F050 MACRO_ADDR;
extern char *D_L00_00166F00;
extern char D_L00_00166F10[];
extern char D_0013E633[];

// Updates the camera mode transition state of an InitCamera moby.
int func_L00_002EB938(char *a, char *b) {
    u128_2EB938 s[3];
    char *g = D_0013E633 + 0xE1D;
    char *v10, *v20, *c1, *c2, *e, *d, *q, *p, *w;
    int r, k, k2, n;
    float f, f20;
    e = *(char **)(D_L00_0015F050 + *(short *)(a + 0x84) * 32 + 0x1C);
    r = *(int *)(g + 0x2284);
    if (r == *(short *)(a + 0x86) || r == 0x51) {
        d = D_L00_00166F00;
        k = *(short *)(b + 0x86);
        if ((k == 0 || k == 0xA || k == 0xE || k == 2) && ((k2 = *(short *)(d + 0x86)) == 0 || k2 == 0xA || k2 == 0xE || k2 == 2)) {
        s[0] = *(u128_2EB938 *)(D_0013E633 + 0xE9D);
        w = (char *)func_L00_001EE530(s, 0.25f);
        if (w != 0 && (*(int *)(w + 0x18) & 1)) return 0;
        *(int *)(e + 0x24) = 0;
        n = *(unsigned short *)(e + 0x20) + 1;
        *(short *)(e + 0x22) = 1;
        *(short *)(e + 0x20) = n;
        if ((short)n >= func_001F9850(7)) {
            if (*(short *)(d + 0x86) == 0) a[0x7C] = 5;
            return 1;
        }
        c1 = D_0013E633 + 0xE1D;
        ((float *)s)[0] = func_001F9F90(*(float *)(c1 + 0x98));
        ((float *)s)[1] = func_001F9FA8(*(float *)(c1 + 0x98));
        ((float *)s)[2] = 0.0f;
        func_L00_002E9DC8(s, 0.06981317f, ((float *)s)[2]);
        } else {
            *(short *)(e + 0x20) = 0;
            *(short *)(e + 0x22) = 0;
        }
    } else {
        *(short *)(e + 0x20) = 0;
        if (*(short *)(e + 0x22) != 0) {
            *(short *)(e + 0x22) = 0;
            p = D_L00_00166F10;
            q = p - 0x190;
            p += 0x30;
            f = func_001F9C78(p, *(char **)(q + 0x180));
            func_001F9C30(s, p, f);
            v10 = (char *)s + 0x10;
            func_001F9BF0(v10, *(char **)(q + 0x180), s);
            f = func_001F9C78(p, *(char **)(g + 0x2080) + 0xC0);
            func_001F9C30(s, p, f);
            v20 = (char *)s + 0x20;
            func_001F9BF0(v20, *(char **)(g + 0x2080) + 0xC0, s);
            f20 = func_001F9CB8(v20);
            if (f20 != 0.0f) {
                f20 = f20 * func_001F9CB8(v10);
                if (f20 != 0.0f) {
                    f = func_001F9FC0(func_001F9C78(v20, v10) / f20);
                    if (1.3962634f <= 1.5707964f - f) {
                        func_L00_001ED600();
                        return 0;
                    }
                }
            }
            *(int *)(e + 0x24) = func_001F9850(0x1E);
        }
    }
tail:
    if (*(int *)(e + 0x24) != 0) {
        func_001F9908((int *)(e + 0x24));
        c2 = D_0013E633 + 0xE1D;
        ((float *)s)[0] = func_001F9F90(*(float *)(c2 + 0x98));
        ((float *)s)[1] = func_001F9FA8(*(float *)(c2 + 0x98));
        ((float *)s)[2] = 0.0f;
        func_L00_002E9DC8(s, 0.13962634f, ((float *)s)[2]);
    }
    return 0;
}
