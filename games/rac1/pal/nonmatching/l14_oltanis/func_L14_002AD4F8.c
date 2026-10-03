/* NON_MATCHING func_L14_002AD4F8 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: BYTES 5/252 (98.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Moby update: picks a 16-byte keyframe vector from D_L14_001B0F30[data[0xB4]] (count*16+tab or tab+0x10 by data
 *   Only difference: register allocation of the table pointer (retail tab in $v0, src in $v1; ours tab in $v1, src
 *   Best candidate p0.c (5 words off).
 */
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern int *D_L14_001B0F30[];

// Pick a keyframe vector from the moby's table and ease the moby position toward it per axis.
void func_L14_002AD4F8(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float v[4];
    char *src;
    int *tab;
    tab = D_L14_001B0F30[*(int *)(data + 0xB4)];
    if (*(float *)(data + 0xA4) > 0.5f) {
        src = (char *)tab + tab[0] * 16;
    } else {
        src = (char *)tab + 0x10;
    }
    qcopy(v, src);
    func_L00_0025C918((float *)(moby + 0x10), (float *)(data + 0xD0), v[0], 0.005f, 0.2f, 0.0f);
    func_L00_0025C918((float *)(moby + 0x14), (float *)(data + 0xD4), v[1], 0.005f, 0.2f, 0.0f);
    func_L00_0025C918((float *)(moby + 0x18), (float *)(data + 0xD8), v[2], 0.005f, 0.2f, 0.0f);
}
