/* NON_MATCHING func_L02_002D9D48 -- src/overlays/shared/vendor_002A5218.c
 * Best so far: SIZE ours 440 / retail 444, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Computes camera position/rotation out params: orbit around moby (idx==-1) or copy two 16-byte entries from a t
 *   Best is p5.c (432 vs 444 bytes): left differences are the schedule of mov.s $f23,$f22 and retail not CSEing (i
 *   Would need the right form of the table address expression (shift+base recomputed) and the f23=f22 copy kept be
 */
typedef int u128 __attribute__((mode(TI)));
extern char D_0013E633[];
extern char D_L02_00167300[];
extern char *D_L02_0016016C;
extern float func_001FA748(float, float);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);

// Computes a camera position and rotation, either orbiting the moby or from a table entry.
void func_L02_002D9D48(float *out, float *rot, char *moby) {
    float tmp[4];
    int idx = *(int *)(*(char **)(moby + 0x78) + 0xE8);
    if (idx == -1) {
        float a, b, c, d, e, f;
        a = func_001FA748(*(float *)(moby + 0x48), 1.5707964f);
        b = func_001FA748(*(float *)(moby + 0x48), -1.5707964f);

        d = func_L00_001FF860(*(float *)(D_L02_00167300 + 0x140) - *(float *)(moby + 0x10),
                              *(float *)(D_L02_00167300 + 0x144) - *(float *)(moby + 0x14));
        e = func_001FA850(d, a);
        f = func_001FA850(d, b);
        c = (e < f) ? a : b;
        func_001F9BD8(tmp, D_0013E633 + 0xE9D, moby + 0x10);
        func_001F9C30(tmp, tmp, 0.5f);
        qcopy(out, tmp);
        out[0] += 2.0f * func_001F9F90(c);
        out[1] += 2.0f * func_001F9FA8(c);
        out[2] += 1.0f;
        *(int *)&rot[0] = 0;
        *(int *)&rot[1] = 0;
        rot[2] = func_001FA748(c, 3.1415927f);
    } else {
        *(u128 *)out = *(u128 *)((char *)(idx << 7) + (int)D_L02_0016016C + 0x30);
        *(u128 *)rot = *(u128 *)((char *)(idx << 7) + (int)D_L02_0016016C + 0x70);
    }
}
