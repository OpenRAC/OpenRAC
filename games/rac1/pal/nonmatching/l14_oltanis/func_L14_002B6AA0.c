/* NON_MATCHING func_L14_002B6AA0 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: BYTES 8/280 (97.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby-style helper: when moby state is 4-6 or 8, eases moby pos x/y/z toward target[0..2] via func_L00_00
 *   Only difference (8 bytes): in the last call ours emits `addiu $a1,$s0,0xE4` before `mov.s $f12,$f26`, retail t
 */
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern void func_L00_002592B0(char *moby, float *vel, float target, float k, float d, float max);

/* eases a moby's position and heading toward targets while in states 4-6 or 8 */
void func_L14_002B6AA0(char *moby, float *target, float a, float b, float c, float d, float e, float f, float g) {
    int data;
    int s = moby[0x20] & 0xFF;
    if ((s >= 4 && s <= 6) || s == 8) {
        data = *(int *)(moby + 0x78);
        func_L00_0025C918((float *)(moby + 0x10), (float *)(data + 0xF0), target[0], b, c, d);
        func_L00_0025C918((float *)(moby + 0x14), (float *)(data + 0xF4), target[1], b, c, d);
        func_L00_0025C918((float *)(moby + 0x18), (float *)(data + 0xF8), target[2], b, c, d);
        func_L00_002592B0(moby, (float *)(data + 0xE4), a, e, f, g);
    }
}
