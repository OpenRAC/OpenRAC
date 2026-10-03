/* NON_MATCHING func_L18_002F33D8 -- src/overlays/l18_veldin2/vendor_002F2AE0.c
 * Best so far: BYTES 65/824 (92.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## Round 1
 *   Particle/quad effect update (moby 0x78 state d: d[2] scroll phase, d[3] alpha cap). Builds 4 Quads (0x90 each:
 *   - Existing `extern void func_L18_002F33D8(void)` (used as callback by 3268) conflicts with a (char*) definitio
 *   - Mattered: `for (i=0;i<4;i=n)` with `n=i+1` in body kills outer strength reduction (868->812 bytes); locals `
 *   - Remaining (p7, same size, 76 bytes): float reg swap (retail ofs=$f20, t=$f21; ours reversed, priority) and i
 */
#include "common.h"
typedef float QVx[4] __attribute__((aligned(16)));
typedef struct {
    QVx v;
} QRx;
typedef struct {
    QRx m[4];
    int colors[4];
    float uv[4][2];
    unsigned long pkt[4];
} Quadx;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L18_00162358;
extern short D_L18_0016235C;
extern short D_L18_00162360;
extern short D_L18_00162364;
extern short D_L18_00162370;
extern short D_L18_00162374;
extern short D_L18_00162378;
extern short D_L18_0016237C;
extern short D_L18_00162380;
extern char D_L18_00167840[];
extern float D_L18_001DA620[][2];
extern QRx D_L18_001DA640[];
extern void func_001FA460(void *, void *);
extern float func_001F9D10(void *, void *);
extern int func_001F4868(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001FA8A8(int, int, float);
extern void func_L00_001FD1D8(void *, void *, int);

void func_L18_002F33D8_a(char *moby) __asm__("func_L18_002F33D8");
void func_L18_002F33D8_a(char *moby) {
    Quadx q[4];
    float mat[4][4];
    float *d = *(float **)(moby + 0x78);
    float lim;
    float quarter;
    float sc;
    int i;
    int n;
    int j;

    func_001FA460(mat, moby + 0xC0);
    qcopy(mat[3], moby + 0x10);
    mat[3][3] = 1.0f;
    d[2] += *(float *)&D_L18_00162370 * D_0015EE6C;
    if (d[2] > 1.0f) {
        d[2] -= 1.0f;
    }
    lim = (48.0f - func_001F9D10(moby + 0x10, D_L18_00167840)) * 0.25f;
    if (lim > d[3]) {
        lim = d[3];
    } else if (lim < 0.0f) {
        lim = 0.0f;
    }
    quarter = 0.25f;
    for (i = 0; i < 4; i = n) {
        float t;
        int c;
        q[i].pkt[1] = func_001F4868(14);
        q[i].pkt[0] = 0;
        q[i].pkt[2] = 0xFF9000000260;
        q[i].pkt[3] = (*(int *)&D_L18_00162374) | (long)(*(int *)&D_L18_00162378) << 2 |
                      (long)(*(int *)&D_L18_0016237C) << 4 | (long)(*(int *)&D_L18_00162380) << 6 |
                      (long)0x8000 << 24;
        t = d[2] + (float)i * quarter;
        t -= (float)func_001FA898_r(t);
        c = func_001FA8A8(*(int *)&D_L18_00162358, *(int *)&D_L18_0016235C, t);
        c = func_001FA8A8(c & 0xFFFFFF, c, lim);
        n = i + 1;
        sc = *(float *)&D_L18_00162364;
        for (j = 0; j < 4; j++) {
            q[i].uv[j][0] = D_L18_001DA620[j][0] + (float)i * quarter;
            q[i].uv[j][1] = D_L18_001DA620[j][1];
            q[i].colors[j] = c;
            qcopy(&q[i].m[j], &D_L18_001DA640[j]);
            if (j & 1) {
                q[i].m[j].v[2] += t * sc;
            } else {
                q[i].m[j].v[2] -= sc * t;
            }
        }
    }
    for (i = 0; i < 3; i++) {
        mat[3][2] += *(float *)&D_L18_00162360;
        func_L00_001FD1D8(&q[0], mat, 0);
        func_L00_001FD1D8(&q[1], mat, 0);
        func_L00_001FD1D8(&q[2], mat, 0);
        func_L00_001FD1D8(&q[3], mat, 0);
    }
}
