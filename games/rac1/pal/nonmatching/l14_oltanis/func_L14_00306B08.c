/* NON_MATCHING func_L14_00306B08 -- src/overlays/l14_oltanis/vendor_002FF358.c
 * Best so far: SIZE ours 220 / retail 216, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Eases a blend factor from func_001FA888 * d[2] through func_L00_00258E58, then lerps the y (+0x18) of each mob
 *   Only difference: 7 instructions, the order of the two hoisted lui (D_L14_001EE920 first in retail, D_L14_00160
 *   Would need an allocator/pseudo-numbering tie-break I could not find.
 */
extern float func_001FA888(int);
extern float func_L00_00258E58(float, float, float, float, float);
extern short *D_L14_001AC2C0[];
extern float D_L14_001EE920[];
extern char *D_L14_00160098;

// Blends the y position of each listed moby between two table values by an eased progress.
void func_L14_00306B08(char *moby) {
    int *d = *(int **)(moby + 0x78);
    short *p = D_L14_001AC2C0[d[0]];
    float *e = D_L14_001EE920;
    char *base;
    if (p != 0) {
        float t = func_001FA888(d[1]);
        float k = func_L00_00258E58(-1.0f, 0.0f, 1.0f, 0.0f, 1.0f - t * *(float *)&d[2]);
        base = D_L14_00160098;
        do {
            float x = *e++;
            char *o = base + ((*(unsigned short *)p & 0x7FFF) << 8);
            float y = x - 20.0f;
            *(unsigned short *)(o + 0x34) &= 0xFFBE;
            *(float *)(o + 0x18) = y + (x - y) * k;
        } while (*p++ >= 0);
    }
}
