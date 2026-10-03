/* NON_MATCHING func_L12_002E41C8 -- src/overlays/l12_hoven/vendor_002C0310.c
 * Best so far: SIZE ours 480 / retail 476, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Hoven path-following step (moby, float target): copies pos, calls 2592B0/59B88, probes 62BC0/25A778, counts fr
 *   p1.c best (480 vs 476): gcc hoists moby+0x10 into an extra saved reg and takes the 0x30/0x40/0x50 buffer addre
 */
#include "common.h"
extern void func_L12_002E2B88(void *);
extern int func_L00_00262BC0(int, void *, void *, void *);
extern void func_L00_002592B0(char *moby, float *vel, float target, float k, float d, float max);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_L00_00259B88(char *, char *, float *, char *, float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_0025A778(void *, void *, int);
extern float func_001F9D10(void *, void *);
extern int func_001F9850(int);
extern int *D_L12_001B0C30[];
extern float D_0015EE6C MACRO_ADDR;

// steers a moby along a path using its velocity and returns a status code
int func_L12_002E41C8(char *moby, float target) {
    float pos[4];
    float vel[4];
    float out[4];
    float res[4];
    float w[4];
    float v[4];
    char *d;
    int r;
    d = *(char **)(moby + 0x78);
    qcopy(pos, moby + 0x10);
    func_L12_002E2B88(moby);
    func_L00_002592B0(moby, (float *)(d + 0x2A0), target, 0.05f, 0.3f, 0.2f);
    vel[0] = func_001F9F90(*(float *)(moby + 0x48)) * 2;
    vel[1] = func_001F9FA8(*(float *)(moby + 0x48)) * 2;
    vel[2] = 0;
    r = func_L00_00259B88(moby, d + 0x180, vel, (char *)out, 1.0f);
    if (*(int *)(d + 0x2D0) != -1) {
        float *vp = v;
        float *wp = w;
        func_001F9BF0(vp, pos, moby + 0x10);
        func_L00_001FF4B0(vp, vp, 0.05f);
        func_001F9BD8(wp, pos, vp);
        if (func_L00_00262BC0(*(int *)(d + 0x2D0), moby + 0x10, wp, res)) {
            int *tbl = D_L12_001B0C30[*(int *)(d + 0x2D0)];
            if (func_L00_0025A778(moby + 0x10, tbl + 4, *tbl) == 0) {
                qcopy(moby + 0x10, res);
            }
            if (func_001F9D10(moby + 0x10, pos) < D_0015EE6C * 0.5f) {
                *(int *)(d + 0x2D4) = *(int *)(d + 0x2D4) + 1;
                if (func_001F9850(10) < *(int *)(d + 0x2D4)) r = 2;
            } else {
                *(int *)(d + 0x2D4) = 0;
            }
        }
    }
    return r;
}
