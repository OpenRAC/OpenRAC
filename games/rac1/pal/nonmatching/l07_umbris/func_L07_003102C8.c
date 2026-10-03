/* NON_MATCHING func_L07_003102C8 -- src/overlays/l07_umbris/vendor_002CE470.c
 * Best so far: SIZE ours 512 / retail 524, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Heading search for a moby: tries its current angle, then sweeps by steps of func_001F9B88 until a clear direct
 *   Left: retail keeps pos in $s0 plus a copy in $s3 (extra saved register, 12 bytes bigger) and lays the early `b
 */
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_0025A778(void *, void *, int);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern float func_001F9B88(float);
extern float func_001FA748(float, float);
extern float func_L00_001FF860(float, float);

// Picks a heading for a moby: its current one if clear, otherwise sweeps for a free direction.
void func_L07_003102C8(char *moby, char *b) {
    float v[4];
    char *pos;
    char *pos2;
    float ang, step, k;
    if (*(int *)(b + 0x134) != 0) {
    pos = moby + 0x10;
    v[0] = func_001F9F90(*(float *)(moby + 0x48)) * 5.2f;
    pos2 = pos;
    v[1] = func_001F9FA8(*(float *)(moby + 0x48)) * 5.2f;
    v[2] = 0.0f;
    func_001F9BD8(v, v, pos);
    if (func_L00_0025A778(v, *(char **)(b + 0x128) + 0x10, **(int **)(b + 0x128)) != 0
        && func_L00_001EFFF0(pos2, v, 0x24, moby, 0) == 0
        && func_L00_001F10E0(1.5f, v, 0x24, moby) == 0) {
        return;
    }
    k = 0.0f;
    step = func_001F9B88(*(float *)(b + 0x140));
    ang = *(float *)(moby + 0x48);
    do {
        ang = func_001FA748(ang, *(float *)(b + 0x140));
        v[0] = func_001F9F90(ang) * 5.2f;
        v[1] = func_001F9FA8(ang) * 5.2f;
        v[2] = 0.0f;
        func_001F9BD8(v, v, pos2);
        if (func_L00_0025A778(v, *(char **)(b + 0x128) + 0x10, **(int **)(b + 0x128)) != 0
            && func_L00_001EFFF0(pos2, v, 0x24, moby, 0) == 0
            && func_L00_001F10E0(1.5f, v, 0x24, moby) == 0) {
            *(float *)(b + 0x13C) = ang;
            return;
        }
        k += step;
    } while (k < 6.2831855f);
    }
    {
        *(float *)(b + 0x13C) = func_L00_001FF860(*(float *)(b + 0xF0) - *(float *)(moby + 0x10), *(float *)(b + 0xF4) - *(float *)(moby + 0x14));
    }
}
