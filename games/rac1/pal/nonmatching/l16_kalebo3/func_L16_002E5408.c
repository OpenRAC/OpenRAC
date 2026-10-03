/* NON_MATCHING func_L16_002E5408 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: SIZE ours 372 / retail 368, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L16_002E5408: picks a path index (state 5 ? data[0xDC] : data[0xD8]), calls func_L00_0025E860 (float arg 
 *   Only diff: constant placement. Retail emits mtc1 $0,$f20 after the 25E860 call and the 0.01f (f21) after the f
 *   Further p5-p6 trials changed the path-table local's scope and the zero constant's scope. Best remains p1/p2/p3
 */
extern int func_L00_0025E860(char *, float *, float *, float *, int, float);
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9B88(float);
extern float func_L00_001FF860(float, float);
extern void func_L00_002592B0(char *moby, float *vel, float target, float k, float d, float max);
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L16_001B0C30[];
extern short D_L16_00161E50, D_L16_00161E54, D_L16_00161E58;

// Steers the moby toward a path point and turns it to face its velocity, returning the path result.
int func_L16_002E5408(char *moby) {
    float pos[4];
    float delta[4];
    char *data = *(char **)(moby + 0x78);
    float t = *(float *)&D_L16_00161E50 * D_0015EE6C;
    int idx;
    int r;
    if ((unsigned char)moby[0x20] == 5)
        idx = *(int *)(data + 0xDC);
    else
        idx = *(int *)(data + 0xD8);
    r = func_L00_0025E860(D_L16_001B0C30[idx], pos, (float *)(data + 0xD0), (float *)(data + 0xD4), 0, t);
    func_L00_0025C918((float *)(moby + 0x10), (float *)(data + 0xE4), pos[0], *(float *)&D_L16_00161E54, *(float *)&D_L16_00161E58, 0.0f);
    func_L00_0025C918((float *)(moby + 0x14), (float *)(data + 0xE8), pos[1], *(float *)&D_L16_00161E54, *(float *)&D_L16_00161E58, 0.0f);
    func_L00_0025C918((float *)(moby + 0x18), (float *)(data + 0xEC), pos[2], *(float *)&D_L16_00161E54, *(float *)&D_L16_00161E58, 0.0f);
    func_001F9BF0(delta, pos, moby + 0x10);
    if (func_001F9B88(delta[0]) > 0.01f) {
        if (func_001F9B88(delta[1]) > 0.01f) {
            func_L00_002592B0(moby, (float *)(data + 0xF0), func_L00_001FF860(delta[0], delta[1]), *(float *)&D_L16_00161E54, *(float *)&D_L16_00161E58, 0.0f);
        }
    }
    return r;
}
