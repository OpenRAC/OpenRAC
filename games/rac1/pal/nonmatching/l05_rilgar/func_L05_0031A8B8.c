/* NON_MATCHING func_L05_0031A8B8 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: SIZE ours 360 / retail 356, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Sets a moby's animation-entry pointer (data+0xAC) from a 5-way switch on data+0xB4, then derives two per-tick 
 *   Switch half matches; only the tail scheduling and two register numbers differ (retail issues both D loads and 
 *   Would need a wording that reorders the float scheduling; not found.
 */
extern float func_001F9D10(void *, void *);
extern char *D_L05_001B0CB0[];
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;

/* Picks the animation entry for the moby's current phase and derives its per-tick rates from the entry length. */
void func_L05_0031A8B8(char *moby) {
    char *data = *(char **)(moby + 0x78);
    switch (*(short *)(data + 0xB4)) {
    case 0:
        *(char **)(data + 0xAC) = D_L05_001B0CB0[*(int *)(data + 0x94)];
        *(short *)(data + 0xB6) = 1;
        break;
    case 1:
        *(char **)(data + 0xAC) = D_L05_001B0CB0[*(int *)(data + 0x98)];
        *(short *)(data + 0xB6) = 0;
        break;
    case 2:
        if (*(unsigned char *)(*(int *)(data + 0xB8) + D_0015EE84_m * 16 + D_0014171B + 0xAA35) != 0xFF) {
            *(char **)(data + 0xAC) = D_L05_001B0CB0[*(int *)(data + 0x9C)];
            *(short *)(data + 0xB6) = 3;
        } else {
            *(char **)(data + 0xAC) = D_L05_001B0CB0[*(int *)(data + 0xA4)];
            *(short *)(data + 0xB6) = 4;
        }
        break;
    case 3:
        *(char **)(data + 0xAC) = D_L05_001B0CB0[*(int *)(data + 0xA0)];
        *(short *)(data + 0xB6) = 2;
        break;
    case 4:
        *(char **)(data + 0xAC) = D_L05_001B0CB0[*(int *)(data + 0xA8)];
        *(short *)(data + 0xB6) = 2;
        break;
    }
    {
        char *e = *(char **)(data + 0xAC);
        float len = func_001F9D10(e + 0x10, e + 0x20) * (float)*(int *)e;
        float inv;
        *(int *)(data + 0xC4) = 0;
        *(int *)(data + 0xC0) = 0;
        inv = 1.0f / len;
        *(float *)(data + 0xD8) = D_0015EE70 * 5.0f * inv;
        *(float *)(data + 0xD4) = D_0015EE6C * 20.0f * inv;
    }
}
