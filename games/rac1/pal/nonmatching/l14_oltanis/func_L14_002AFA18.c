/* NON_MATCHING func_L14_002AFA18 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 344 / retail 292, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Sums per-entry floats (stride 0x10 from table D_L14_001B0F30[data->0x78]) plus data->0x1CC, scales by data->0x
 *   Best: p6.c (42 insn-lines differ but all in scheduling): everything matches except the two tmp stores (swc1 $f
 *   Unblock: whatever makes the scheduler sink those stores/mtc1 past the div.s; tried arg local, stores before/af
 */
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern short D_L14_00161468;
extern short D_L14_0016146C;

// Sums per-entry values and a bias, scales, then eases the moby position toward the result.
void func_L14_002AFA18(char *moby)
{
    char *data = *(char **)(moby + 0x78);
    float out[3];
    int ia;
    float fb;
    float sum = 0.0f;
    char *tab = D_L14_001B0F30[*(int *)(data + 0x78)];
    int n = *(int *)(data + 0x84);
    int i;
    float *p;

    if (n > 0) {
        p = (float *)(tab + 0x1C);
        i = n;
        do {
            sum += *p;
            p += 4;
        } while (--i != 0);
    }
    sum += *(float *)(data + 0x1CC);
    fb = 0.0f;
    ia = 0;
    func_L00_0025E860(D_L14_001B0F30[*(int *)(data + 0x74)], out, (float *)&ia, &fb,
                      *(short *)(data + 0x8A),
                      sum / *(float *)(data + 0x16C) * *(float *)(data + 0x190));
    func_L00_0025C918((float *)(moby + 0x10), (float *)(data + 0x194), out[0],
                      *(float *)&D_L14_00161468, *(float *)&D_L14_0016146C, 0.0f);
    func_L00_0025C918((float *)(moby + 0x14), (float *)(data + 0x198), out[1],
                      *(float *)&D_L14_00161468, *(float *)&D_L14_0016146C, 0.0f);
    func_L00_0025C918((float *)(moby + 0x18), (float *)(data + 0x19C), out[2],
                      *(float *)&D_L14_00161468, *(float *)&D_L14_0016146C, 0.0f);
}
