extern char D_0013E633[];
extern char D_L00_00179990[];
extern int D_L00_00179BB0 NOT_SDA;
extern int D_0015EE84 NOT_SDA;
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern int func_L00_0020DB30(int);
extern int func_001F9850(int);
extern int func_L00_001EFFF0(void *, void *, int, int, int);

typedef u32 vec128_2297E0 __attribute__((mode(TI), aligned(16)));

/* Picks which of the five table rows the hero should act on: rows whose timers and flags allow it are
   listed in the array at sp, the list is weighted by 60/x per row, and the choice is written to *out.
   Returns 1 when a row was picked, 0 when none qualifies. */
int func_L00_002297E0(int *out) {
    unsigned char *h = (unsigned char *)D_0013E633 + 0xE1D;
    int idx[5];
    vec128_2297E0 a;
    vec128_2297E0 b;
    int *p;
    char *r;
    int i;
    int count;
    int v;
    int r4;
    int r2;
    int rr;
    float f0;
    float f20;
    float f21;
    float f22;
    float f23;

    if (*(short *)(h + 0x1E8) != 0) {
        return 0;
    }
    if (*(int *)(h + 0x22A8) == 1) {
        *out = 4;
        if (D_L00_00179BB0 != 0) {
            return 0;
        }
        if ((*(int *)(h + 0xA98) & 2) == 0) {
            return 0;
        }
        f0 = func_002140F8(0.0f, 1.0f);
        return (f0 < 0.33f) ? 1 : 0;
    }

    count = 0;
    p = idx;
    for (i = 0; i < 5; i++) {
        r = D_L00_00179990 + i * 0x70;
        if (*(int *)(r + 0x60) != 0) {
            continue;
        }
        if (*(float *)(r + 0x58) == 0.0f) {
            continue;
        }
        r4 = func_L00_0020DB30(0);
        if (*(int *)(r + 0x50) != -1 && *(int *)(r + 0x50) != r4) {
            continue;
        }
        v = *(int *)(r + 0x44);
        if (v != 0 && *(int *)(h + 0x2FC) != 0) {
            continue;
        }
        if (i == 3) {
            if (D_0015EE84 != 12) {
                continue;
            }
            if (func_L00_0020DB30(0) == 0x17) {
                continue;
            }
            a = *(vec128_2297E0 *)(h + 0xD0);
            b = a;
            *(float *)((char *)&b + 8) = *(float *)((char *)&b + 8) + 8.0f;
            r2 = func_L00_001EFFF0(&a, &b, 2, *(int *)(h + 0x2080), 0);
            if (r2 != 0) {
                continue;
            }
        }
        if (i == 4) {
            if (*(int *)(h + 0x2094) != 1) {
                continue;
            }
            rr = func_001F9850(50);
            if (rr < *(int *)(h + 0x198)) {
                continue;
            }
            if (!(*(int *)(h + 0x22A8) < 2)) {
                continue;
            }
        }
        *p = i;
        count++;
        p++;
    }
    if (count == 0) {
        return 0;
    }

    f20 = 0.0f;
    f22 = 60.0f;
    f21 = 1.0f;
    p = idx;
    for (i = count; i != 0; i--) {
        r = D_L00_00179990 + *p * 0x70;
        p++;
        f0 = func_001F9878(*(float *)(r + 0x58));
        f0 = f0 * f22;
        f0 = (float)(int)f0;
        f0 = f21 / f0;
        f20 = f20 + f0;
    }

    f22 = 1.0f;
    f21 = func_002140F8(0.0f, f22);
    if (!(f21 < f20)) {
        return 0;
    }
    f23 = 60.0f;
    p = idx;
    for (i = 0;; i++) {
        if (!(i < count)) {
            return 1;
        }
        *out = *p;
        r = D_L00_00179990 + *p * 0x70;
        f0 = func_001F9878(*(float *)(r + 0x58));
        f0 = f0 * f23;
        f0 = (float)(int)f0;
        f0 = f22 / f0;
        f20 = f20 - f0;
        if (f20 < f21) {
            return 1;
        }
        p++;
    }
}
