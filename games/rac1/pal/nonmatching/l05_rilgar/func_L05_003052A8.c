/* NON_MATCHING func_L05_003052A8 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: BYTES 9/304 (97.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Checks whether a point on a circle around the moby (angle from atan2 to target, radius from sqrt/dist of targe
 *   Only 9 of 304 bytes differ: retail loads moby pos x,y (0x10/0x14 off $a0) into $f12/$f13 first and target x,y 
 *   Would need the exact source expression shape for the atan2 arguments; scheduler/allocator tie.
 */
extern float func_L00_001FF860(float, float);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9D10(void *, void *);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern int func_L00_002629E0(int, void *, void *);

/* tests whether a line from a point on a circle around the moby to a target is clear */
int func_L05_003052A8(char *moby, float *target, float scale) {
    float a[4];
    float b[4];
    float *pos = (float *)(moby + 0x10);
    char *data = *(char **)(moby + 0x78);
    float ang = func_001FA748(func_L00_001FF860(target[0] - pos[0], target[1] - *(float *)(moby + 0x14)), *(float *)(data + 0x294) * 1.5f);
    a[0] = func_001F9F90(ang) * func_001F9D10(target, pos);
    a[1] = func_001F9FA8(ang) * func_001F9D10(target, pos);
    a[2] = 0;
    func_001F9BD8(a, a, pos);
    func_001F9BF0(b, a, target);
    func_L00_001FF4B0(b, b, scale);
    func_001F9BD8(b, b, pos);
    return func_L00_002629E0(*(int *)(data + 0x2C0), a, b) == 0;
}
