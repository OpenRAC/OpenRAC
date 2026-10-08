/* NON_MATCHING func_L10_002E6FA0 -- src/overlays/l10_orxon/vendor_002E30F8.c
 * Best so far: BYTES 70/732 (90.4% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern short D_L10_00161F70, D_L10_00161F74, D_L10_00161F78, D_L10_00161F7C, D_L10_00161F80, D_L10_00161F84;
extern void func_00215C00(void *, float, float, float);
extern void func_001F9C30(void *, void *, float);
extern int func_001FA8A8(int, int, float);
extern int func_L00_00274788_x(void *, void *, int, float, float, float, int, int, int) __asm__("func_L00_00274788");

/* Exhaust vent: each frame emits a burst of smoke puffs from a random spot across the vent, blown out
 * along its facing; some are small fast sparks instead. */
void func_L10_002E6FA0(char *m) {
    float pos[4];
    float vel[4];
    float off[4];
    char *d = *(char **)(m + 0x78);
    char *mp = m + 0x10;
    int i;
    ((unsigned char *)m)[0x30] = 0x80;
    for (i = 0; i < *(int *)(d + 0xC); i++) {
        int spark = func_002140B0(100) < *(int *)&D_L10_00161F84;
        float a, b, c;
        float lo, hi;
        float s;
        int c1, c2;
        qcopy(pos, mp);
        off[0] = func_001F9F90(func_001FA748(*(float *)(m + 0x48), 1.5707964f)) * func_002140F8(-*(float *)(d + 0x10), *(float *)(d + 0x10));
        off[1] = func_001F9FA8(func_001FA748(*(float *)(m + 0x48), 1.5707964f)) * func_002140F8(-*(float *)(d + 0x10), *(float *)(d + 0x10));
        off[2] = 0.0f;
        off[2] = func_002140F8(-*(float *)(d + 0x14), *(float *)(d + 0x14));
        func_001F9BD8(pos, pos, off);
        a = func_002140F8(*(float *)&D_L10_00161F7C, *(float *)&D_L10_00161F78);
        b = func_002140F8(*(float *)&D_L10_00161F7C, *(float *)&D_L10_00161F78);
        c = func_002140F8(*(float *)&D_L10_00161F7C, *(float *)&D_L10_00161F78);
        if (a < b) a = b;
        if (a < c) a = c;
        func_00215C00(vel, a * D_0015EE6C, *(float *)(m + 0x48), -*(float *)(m + 0x44));
        if (spark) {
            func_001F9C30(vel, vel, func_002140F8(0.5f, 1.5f));
            lo = 0.24f;
            hi = 0.36f;
        } else {
            lo = 0.8f;
            hi = 1.2f;
        }
        s = *(float *)(d + 4) * func_002140F8(lo, hi);
        c1 = func_001FA8A8(*(int *)&D_L10_00161F70, *(int *)&D_L10_00161F74, func_002140F8(0.5f, 1.0f));
        c2 = func_001FA8A8(*(int *)&D_L10_00161F74, *(int *)&D_L10_00161F74 & 0xFF000000, func_002140F8(0.25f, 0.5f));
        func_L00_00274788_x(pos, vel, func_001FA898(func_001F9878(*(float *)(d + 8) * 60.0f)), s, *(float *)d, *(float *)&D_L10_00161F80 * D_0015EE70, c1, c2, spark);
    }
}
