/* NON_MATCHING func_L18_002DCE10 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 75/1116 (93.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   The jal func_001F9878 'diff' (207C70 vs 207C28) is the nearest-place heuristic display, not a real mismatch.
 *   Tried: mp pointer for the matrix (spills), j reused as final counter (worse), fk copies. Best = p5.c (456/1116
 *   ## Round 2 (retry, 12 of 14 runs)
 *   Tried: local order (n,k,j) = same bytes; K as a float local (size 1100, worse); m declared before q (no gain);
 *   everywhere / only call+loop (448/440, worse); m[3].f[3] store reordered (same 456); `d = moby->data` assigned 
 *   func_001FA190 call instead of at declaration (p15, 468: best, +12 bytes); d moved before the loop (p16, same 4
 *   Retail keeps &m in $s1 from the prologue (addiu s1,sp,0x240 before the first call); no variant made GCC hoist 
 *   Remaining: s0/s1 assignment, prologue spill order, mov.s $f24,$f20 + K in $f23.
 */
#include "common.h"
typedef struct { float f[4]; } __attribute__((aligned(16))) V_dce10;
typedef struct { float u, v; } UV_dce10;
typedef struct {
    V_dce10 corner[4];
    unsigned int color[4];
    UV_dce10 uv[4];
    long unk70, tex, unk80, unk88;
} Q_dce10;
typedef struct {
    char pad0[0x20];
    float f20, f24, f28;
} D_dce10;
typedef struct {
    char pad0[0x10];
    V_dce10 pos;
    char pad20[0x58];
    D_dce10 *data;
} M_dce10;

extern void func_001FA190(void *);
extern int func_001F4868(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001FA8A8(int, int, float);
extern float func_001F9878(float);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BC0(void *);
extern void func_001FA1F8(void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern void func_001FA4F0(void *, void *, void *);

extern short D_L18_00161CD8;
extern short D_L18_00161CDC;
extern short D_L18_00161CE0;
extern short D_L18_00161CE4;
extern short D_L18_00161CEC;
extern short D_L18_00161CF0;
extern short D_L18_00161CF4;
extern short D_L18_00161CF8;
extern short D_L18_00161CFC;
extern short D_L18_00161D00;
extern short D_L18_00161D04;
extern short D_L18_00161D08;
extern short D_L18_00161D0C;
extern float D_L18_001D45E0[];

void func_L18_002DCE10(M_dce10 *moby) {
    Q_dce10 q[4];
    V_dce10 m[4];
    V_dce10 rot[3];
    V_dce10 rv;
    D_dce10 *d = moby->data;
    int j, k, n;
    int col;

    func_001FA190(m);
    qcopy(&m[3], &moby->pos);
    m[3].f[2] += *(float *)&D_L18_00161CF8;
    m[3].f[3] = 1.0f;
    for (j = 0; j < 4; j++) {
        float fj;
        float t, ang, rad;
        q[j].tex = func_001F4868(0xE);
        q[j].unk70 = 0;
        q[j].unk80 = 0xFF9000000260L;
        q[j].unk88 = (long)*(int *)&D_L18_00161CD8 | ((long)*(int *)&D_L18_00161CDC << 2) | ((long)*(int *)&D_L18_00161CE0 << 4) | ((long)*(int *)&D_L18_00161CE4 << 6) | 0x8000000000L;
        t = d->f24 * *(float *)&D_L18_00161D04 + (float)j * 0.25f;
        rad = d->f24 + *(float *)&D_L18_00161D00 * (float)j;
        t = t - (float)func_001FA898_r(t);
        col = func_001FA8A8(*(int *)&D_L18_00161CEC, *(int *)&D_L18_00161CF0, t);
        ang = t * *(float *)&D_L18_00161D08;
        if (ang > 1.0f) ang = 1.0f;
        col = func_001FA8A8(col & 0xFFFFFF, col, ang);
        ang = (d->f28 - d->f24) / (d->f20 * func_001F9878(*(float *)&D_L18_00161D0C));
        if (ang > 1.0f) {
            ang = 1.0f;
        } else if (ang < 0.0f) {
            ang = 0.0f;
        }
        col = func_001FA8A8(col & 0xFFFFFF, col, ang);
        fj = (float)j;
        for (k = 0; k < 4; k++) {
            float a;
            q[j].uv[k].u = D_L18_001D45E0[2 * k];
            q[j].uv[k].v = D_L18_001D45E0[2 * k + 1];
            if (k & 1) {
                q[j].corner[k].f[2] = t * *(float *)&D_L18_00161CFC;
            } else {
                q[j].corner[k].f[2] = -t * *(float *)&D_L18_00161CFC;
            }
            a = func_001FA748((float)(k >> 1) * (*(float *)&D_L18_00161CF4 * 0.017453292f) - 3.14159f,
                              *(float *)&D_L18_00161CF4 * 0.25f * fj * 0.017453292f);
            q[j].corner[k].f[0] = func_001F9F90(a) * rad;
            q[j].corner[k].f[1] = func_001F9FA8(a) * rad;
            q[j].corner[k].f[3] = 1.0f;
            q[j].color[k] = col;
        }
    }
    func_001F9BC0(&rv);
    rv.f[2] = *(float *)&D_L18_00161CF4 * 0.017453292f;
    func_001FA1F8(rot, &rv);
    for (n = 0; (float)n < 360.0f / *(float *)&D_L18_00161CF4; n++) {
        func_L00_001FD1D8(&q[0], m, 0);
        func_L00_001FD1D8(&q[1], m, 0);
        func_L00_001FD1D8(&q[2], m, 0);
        func_L00_001FD1D8(&q[3], m, 0);
        func_001FA4F0(m, m, rot);
    }
}
