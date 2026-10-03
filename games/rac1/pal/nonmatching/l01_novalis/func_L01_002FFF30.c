/* NON_MATCHING func_L01_002FFF30 -- src/overlays/l01_novalis/vendor_002FABE8.c
 * Best so far: SIZE ours 512 / retail 524, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   PulsingMeshUpdate: one-time init (gp flag) shifts vertex floats of 4 mesh entries (stride 0x60: ptr at 0, coun
 *   Best is p4.c (do-while with `ni = i+1`, d/e two copies of the table pointer, array declared as PulseEntry via 
 */
typedef struct {
    float *p;
    int pad4[2];
    int n;
    char pad10[0x4C];
    float x;
} PulseEntry;
extern PulseEntry D_L01_001FBF40_e[] __asm__("D_L01_001FBF40");
extern short D_L01_00161D70;
extern int D_L01_0015F6B0 MACRO_ADDR;
extern float func_001FA888(int);
extern float func_001F9FA8(float);
extern void func_001F49B0(void *, void *);
extern void func_L01_002FFF00(void);

/* Pulsing mesh update: skews the mesh vertices once, then pulses each entry from a sine of the frame counter. */
void func_L01_002FFF30(void *moby) {
    float *p;
    float v;
    int i, j, k;
    PulseEntry *d = D_L01_001FBF40_e;
    if (*(int *)&D_L01_00161D70 == 0) {
        *(int *)&D_L01_00161D70 = 1;
        for (i = 0; i < 4; i++) {
            float *q = d[i].p;
            for (j = 0; j < D_L01_001FBF40_e[i].n; j++) {
                float t = q[2] - 0.45f;
                q[3] = t;
                q[2] = t;
                q += 4;
            }
        }
        k = 1;
        for (i = 0; i < 4; i++) {
            float *q = d[i].p;
            k--;
            for (j = 0; j < D_L01_001FBF40_e[i].n; j += 2) {
                float a = q[2], b = q[3], c = q[6], d = q[7];
                if (k & 1) {
                    a += 0.05f; b -= 0.05f; c -= 0.05f; d += 0.05f;
                } else {
                    a -= 0.05f; b += 0.05f; c += 0.05f; d -= 0.05f;
                }
                q[2] = a;
                q[3] = b;
                q[6] = c;
                q[7] = d;
                q += 8;
                k++;
            }
        }
    }
    v = func_001F9FA8((func_001FA888(D_L01_0015F6B0 & 0x3F) - 32.0f) * 0.09817477f) * 0.5f + 0.5f;
    p = &D_L01_001FBF40_e[3].x;
    for (i = 3; i >= 0; i--) {
        *p = v;
        p -= 24;
    }
    func_001F49B0(func_L01_002FFF00, moby);
}
