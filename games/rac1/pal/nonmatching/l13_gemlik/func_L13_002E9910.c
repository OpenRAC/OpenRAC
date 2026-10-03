/* NON_MATCHING func_L13_002E9910 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: BYTES 12/544 (97.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_352: five-state (jump table) fade of a moby: eases moby+0x2C toward target*4 or target, ramps data+
 *   Best p1.c: 12 bytes differ, only FP register numbers (D_0015EE60 in f3 / 0.02 const in f2 in ours; retail f2 /
 *   Would unblock: a wording that changes the creation order of the FP constant pseudos (0.02f, 0.1f); t-before/af
 */
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_0020D678(void *);
extern void func_L00_0024FFE8(unsigned char *, int, int);
extern float D_0015EE60 MACRO_ADDR;

/* Update for moby class 352: five-state fade in and out, then scrolls a texture offset. */
void func_L13_002E9910(unsigned char *moby) {
    char *d = *(char **)(moby + 0x78);
    int a, b, dy;
    float t, k;
    int x;
    switch (moby[0x20]) {
    case 0:
        x = *(int *)(*(char **)(moby + 0x24) + 0x10);
        moby[0x20] = 1;
        *(int *)(moby + 0x94) = x;
        break;
    case 1:
        k = D_0015EE60;
        t = k * 0.02f;
        *(float *)(moby + 0x2C) += (*(float *)(*(char **)(moby + 0x24) + 0x24) * 4.0f - *(float *)(moby + 0x2C)) * 0.1f;
        *(float *)(d + 4) += t;
        if (*(float *)(d + 4) >= 1.0f) {
            *(float *)(d + 4) = 1.0f;
            moby[0x20] = 2;
        }
        moby[0x23] = func_001FA898_r(*(float *)(d + 4) * 60.0f);
        break;
    case 2:
        *(float *)(moby + 0x2C) += (*(float *)(*(char **)(moby + 0x24) + 0x24) * 4.0f - *(float *)(moby + 0x2C)) * 0.1f;
        if (*(int *)d == 0 || *(unsigned char *)(*(char **)d + 0x20) == 0xFE ||
            *(unsigned char *)(*(char **)d + 0x20) == 0xFD) {
            moby[0x20] = 3;
        }
        break;
    case 3:
        k = D_0015EE60;
        t = k * 0.02f;
        *(float *)(moby + 0x2C) += (*(float *)(*(char **)(moby + 0x24) + 0x24) - *(float *)(moby + 0x2C)) * 0.1f;
        *(float *)(d + 4) -= t;
        if (*(float *)(d + 4) <= 0.0f) {
            *(float *)(d + 4) = 0.0f;
            moby[0x20] = 4;
        }
        moby[0x23] = func_001FA898_r(*(float *)(d + 4) * 60.0f);
        break;
    case 4:
        func_0020D678(moby);
        return;
    }
    a = *(int *)(d + 8);
    b = a - 0x40;
    dy = -0x40;
    *(int *)(d + 8) = b;
    if (b > 0x1000) {
        dy = -0x1040;
        *(int *)(d + 8) = a - 0x1040;
    } else if (b < 0) {
        dy = 0xFC0;
        *(int *)(d + 8) = a + 0xFC0;
    }
    func_L00_0024FFE8(*(unsigned char **)(moby + 0x24), 0, dy);
}
