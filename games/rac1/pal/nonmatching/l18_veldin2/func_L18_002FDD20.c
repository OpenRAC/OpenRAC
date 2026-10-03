/* NON_MATCHING func_L18_002FDD20 -- src/overlays/l18_veldin2/vendor_002F9D48.c
 * Best so far: BYTES 16/564 (97.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # Round 1
 *   Draws a 4-vertex textured quad in the moby's frame (sibling of func_L18_002F3A80): builds a basis from the mob
 *   Mattered: D_L18_00167840 as `extern int` (lui/addiu, not gp); pointer locals p90/pB0 assigned after the vC0 qc
 *   Remaining (p9, 16 bytes): loop induction-variable init/increment order only (retail s0=m initialised before s2
 */
#include "common.h"
extern void func_001F9BC0(void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern int func_001F4868(int);
extern int func_001FA8A8(int, int, float);
extern void func_L00_001FD1D8(void *, void *, int);
extern int D_L18_00167840;
extern float D_L18_001EFF90[][2];
extern short D_L18_001626BC;
extern short D_L18_001626C0;
extern short D_L18_001626C4;
extern short D_L18_001626C8;
extern short D_L18_001626CC;
extern short D_L18_001626D0;
extern short D_L18_001626D4;
extern short D_L18_001626D8;
extern short D_L18_001626DC;
extern short D_L18_001626E0;

void func_L18_002FDD20(char *moby) {
    float m[4][4];
    int colors[4];
    float uv[4][2];
    unsigned long pkt[4];
    float v90[4];
    float vA0[4];
    float vB0[4];
    float vC0[4];
    char *data = *(char **)(moby + 0x78);
    float f21;
    float *p90;
    float *pB0;
    float f20 = *(float *)(data + 0x1F4);
    int col;
    int i;

    moby += 0x10;
    f21 = 1.0f - f20;
    f20 = f20 + 1.0f;
    f20 += *(float *)&D_L18_001626DC;
    f21 += *(float *)&D_L18_001626E0;
    qcopy(vC0, moby);
    vC0[2] = vC0[2] - *(float *)&D_L18_001626D8;
    pB0 = vB0;
    p90 = v90;
    func_001F9BC0(pB0);
    vB0[2] = f20;
    func_001F9BF0(p90, &D_L18_00167840, moby);
    func_001F9CA0(vA0, p90, pB0);
    func_001F9CA0(p90, vA0, pB0);
    func_L00_001FF4B0(p90, p90, f21);
    func_L00_001FF4B0(vA0, vA0, f21);
    pkt[1] = func_001F4868(*(int *)&D_L18_001626D4);
    pkt[3] = (*(int *)&D_L18_001626BC) | (long)(*(int *)&D_L18_001626C0) << 2 | (long)(*(int *)&D_L18_001626C4) << 4 |
             (long)(*(int *)&D_L18_001626C8) << 6 | 0x8000000000L;
    pkt[2] = 0xFF9000000260;
    pkt[0] = 0;
    col = func_001FA8A8(*(int *)&D_L18_001626CC, *(int *)&D_L18_001626D0, *(float *)(data + 0x1F4));
    for (i = 0; i < 4; i++) {
        float *mp = m[i];
        uv[i][0] = D_L18_001EFF90[i][0];
        uv[i][1] = D_L18_001EFF90[i][1];
        func_001F9BC0(mp);
        mp[2] = (i & 1) ? 1.0f : 0;
        mp[1] = (i < 2) ? 1.0f : -1.0f;
        mp[3] = 1.0f;
        colors[i] = col;
    }
    func_L00_001FD1D8(m, v90, 0);
}
