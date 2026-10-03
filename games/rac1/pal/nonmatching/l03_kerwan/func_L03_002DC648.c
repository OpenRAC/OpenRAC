/* NON_MATCHING func_L03_002DC648 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: SIZE ours 516 / retail 512, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Track-item moby update (UpdateMoby_899). p5.c is closest (46/512 bytes differ): D_0015EE6C as extern float[] a
 */
extern char D_L03_001E38E0[];
extern int D_L03_001B08B0[];
extern float D_0015EE6C MACRO_ADDR;
extern float D_L03_00161C40;
extern int func_001E9730();
extern void func_0020D678(void *);
extern float func_001F9D10(void *, void *);
extern void func_L03_002DC560(void *, float);
extern int func_001F9908(int *arg0);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_L00_0028EB98(void *, int);
extern int func_L00_0028EF68(int i, int a1, int v, int k);

/* update for the moby that spawns and animates the track items */
void func_L03_002DC648(char *m) {
    int *d = *(int **)(m + 0x78);
    char *t;
    int i;
    if (m[0x20] == 0) {
        if (d[0] == -1) {
            func_001E9730(D_L03_001E38E0, *(unsigned short *)(m + 0xA8));
            func_0020D678(m);
            return;
        }
        t = (char *)D_L03_001B08B0[d[0]];
        D_L03_00161C40 = D_0015EE6C * 3.0f;
        for (i = 0; i < *(int *)t - 2; i++) {
            *(float *)(t + 0x1C + i * 16) = func_001F9D10(t + 0x10 + i * 16, t + 0x20 + i * 16);
        }
        d[1] = 0;
        m[0x20] = 1;
        d[2] = -1;
        {
            float a = 2.9f;
            float b;
            if (a < (float)*(int *)t * *(float *)(t + 0x1C)) {
                b = a;
                do {
                    func_L03_002DC560(t, a);
                    a += b;
                } while (a < (float)*(int *)t * *(float *)(t + 0x1C));
            }
        }
        m[0x30] = 0x60;
    }
    if (func_001F9908(d + 1)) {
        d[1] = func_001FA898_r(2.9f / D_L03_00161C40);
        func_L03_002DC560((void *)D_L03_001B08B0[d[0]], 0.0f);
    }
    if (func_L00_0028EB98(m, d[2]) == 0) {
        d[2] = func_L00_0028EF68(0, 4, (int)m, 0x382);
    }
}
