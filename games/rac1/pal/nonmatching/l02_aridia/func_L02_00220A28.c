/* NON_MATCHING func_L02_00220A28 -- src/overlays/l02_aridia/help_0021BC90.c
 * Best so far: BYTES 22/544 (96.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L02_00220A28: picks target colour values by mode (0/3 set 0.8/0.45/0.7|0.6), adjusts by state, then eases
 *   Differences: in the mode 0/3 block retail assigns f0=0.8, f2=0.7|0.6, f1=0.45 (stores f0,f1,f2 in order 228,23
 *   Unblock: a wording that makes the 0.45 constant allocate between a and c (or that restructures the mode test s
 */
extern unsigned char D_0013E633[];
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float func_001F9D48(float *, float *);
extern float func_00214D28(float *p, float target, float maxstep);
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);

/* Picks the target colours for the current mode and eases the live values toward them. */
void func_L02_00220A28(void) {
    char *g = (char *)D_0013E633 + 0xE1D;
    char *g2;
    int mode = *(unsigned char *)(g + 0x20A4);
    int v;

    if (mode == 0 || mode == 3) {
        float a, b, c;
        if (mode == 0) {
            a = 0.8f;
            c = 0.7f;
        } else {
            a = 0.8f;
            c = 0.6f;
        }
        b = 0.45f;
        *(float *)(g + 0x228) = a;
        *(float *)(g + 0x230) = b;
        *(float *)(g + 0x22C) = c;
    }
    g = (char *)D_0013E633 + 0xE1D;
    v = *(int *)(g + 0x208C);
    if (v == 4) {
        if (*(int *)(g + 0x198) > *(int *)(g + 0x420)) {
            if (*(short *)(g + 0x41E) == 0) {
                *(float *)(g + 0x22C) = *(float *)(g + 0x434);
            }
        }
    } else {
        int w = *(int *)(g + 0x2084);
        if (w == 6) {
            *(float *)(g + 0x22C) = 0.5f;
        } else if (w == 4) {
            *(float *)(g + 0x228) = 0.35000002f;
        } else if (w == 0x7F) {
            *(float *)(g + 0x230) = 0.8f;
        }
    }
    g2 = (char *)D_0013E633 + 0xE1D;
    if (*(unsigned char *)(g2 + 0x257) == 0 || *(int *)(g2 + 0x2094) == 0x12 || *(int *)(g2 + 0x208C) == 0x11
        || *(unsigned char *)(g2 + 0x12E4) != 0
        || func_001F9D48((float *)(g2 + 0x210), (float *)(g2 + 0x80)) > *(float *)(g2 + 0x234) * 0.5f) {
        float *w = (float *)(D_0013E633 + 0x103D);
        char *g3 = (char *)D_0013E633 + 0xE1D;
        func_00214D28(w, *(float *)(g3 + 0x228), D_0015EE60 * 0.02f);
        func_00214D28(w + 1, *(float *)(g3 + 0x22C), D_0015EE60 * 0.02f);
        func_L00_0025C918(w + 5, w + 6, *(float *)(g3 + 0x230), D_0015EE64 * 0.02f, D_0015EE64 * 0.3f, D_0015EE6C * 4.0f);
    }
}
