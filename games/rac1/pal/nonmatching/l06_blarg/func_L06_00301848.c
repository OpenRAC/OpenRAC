/* NON_MATCHING func_L06_00301848 -- src/overlays/l06_blarg/vendor_002FE5D0.c
 * Best so far: BYTES 4/408 (99.0% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L06_00301848 (408 B)
 *   Best: m1.c BYTES 4/408. Only the two loop-invariant spill setups (&pk.uv[0] -> 0xD0, &pk.uv[1] -> 0xD4)
 *   are computed in the other order / temp registers. Levers: Pk struct from vendor_002D3DF8.c, explicit
 *   vp/u/v pointer IVs declared v, u, vp (gives retail's s2/s3/s4), e[1] read before the call, pk.a = 0
 *   before func_001FA190 (fills its delay slot), col stores written 3,2,1,0 then M[3][2].
 */
#include "common.h"

typedef struct {
    float m[4][4];
    u32 col[4];
    float uv[8];
    u64 a, b, c, d;
} Pk_301848;
extern short D_L06_001620DC;
extern short D_L06_001620E4;
extern short D_L06_001620E8;
extern short D_L06_001620EC;
extern short D_L06_001620F0;
extern short D_L06_001620F4;
extern short D_L06_001620F8;
extern short D_L06_001620FC;
extern short D_L06_00162104;
extern float D_L06_001F3200[][4];
extern short D_L06_001F3800[][8];
extern float D_L06_001F3C00[][2];
u64 func_001F4868(s32);
extern void func_001FA190(void *);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_001FD1D8(void *, void *, s32);

/* Draws the 64 quads of the model table with the level's texture and colour settings. */
void func_L06_00301848(void) {
    Pk_301848 pk;
    float M[4][4];
    u32 col;
    int i, j;
    pk.b = func_001F4868(*(int *)&D_L06_001620F4);
    pk.d = *(int *)&D_L06_001620E4 | ((long)*(int *)&D_L06_001620E8 << 2) | ((long)*(int *)&D_L06_001620EC << 4)
         | ((long)*(int *)&D_L06_001620F0 << 6) | ((long)*(int *)&D_L06_001620F8 << 32);
    pk.c = 0x0000FF9000000260ULL;
    pk.a = 0;
    func_001FA190(M);
    col = *(int *)&D_L06_001620FC;
    pk.col[3] = col;
    pk.col[2] = col;
    pk.col[1] = col;
    pk.col[0] = col;
    M[3][2] = *(float *)&D_L06_001620DC;
    for (i = 0; i < 64; i++) {
        short *e = D_L06_001F3800[i];
        float *v = &pk.uv[1];
        float *u = &pk.uv[0];
        float *vp = pk.m[0];
        for (j = 0; j < 4; j++) {
            int t;
            t = e[1];
            func_001F9C30(vp, D_L06_001F3200[e[0]], *(float *)&D_L06_00162104);
            *u = D_L06_001F3C00[t][0];
            *v = D_L06_001F3C00[t][1];
            e += 2;
            vp += 4;
            u += 2;
            v += 2;
        }
        func_L00_001FD1D8(&pk, M, 0);
    }
}
