/* NON_MATCHING func_L14_002B4C70 -- src/overlays/shared/vendor_002B2A28.c
 * Best so far: BYTES 30/460 (93.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a particle effect: queues two draw commands, shrinks a position by a clamped distance, fills a 0x70-byt
 *   Left: retail loads g (-0x5734($gp)) into $v1 and the lui into $v0, keeps &m in $s3 with a copy in $s0 (ours re
 *   Store order in source made no difference to p3/p4; would need the real struct/array shape for the matrix.
 */
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L14_001675C0[];
extern char D_L14_001D8B90[];
extern short D_L14_001615C8;
extern short D_L14_001615C4;
extern short D_L14_001615CC;
extern short D_L14_001615C0;

// Builds a particle effect from the moby position: queues draw commands and spawns it.
void func_L14_002B4C70(char *moby) {
    float w[4];
    float m[28];
    long pk[4];
    float v1[4];
    float v2[4];
    float len, s;
    int i;
    float *vp;
    char *tp;
    int g;
    qcopy(w, moby + 0x10);
    w[2] = w[2] + 0.01f;
    w[3] = 1.0f;
    pk[1] = func_001F4868(0xB);
    pk[2] = 0xFF9000000260L;
    pk[3] = 0x8000000048L;
    pk[0] = 0;
    func_00234C98(0x4A, 0);
    func_00234C98(0x47, 0x51001);
    func_001F9BF0(v1, D_L14_001675C0, w);
    len = func_001F9CB8(v1);
    if (len > 0.0f) {
        float x = len - *(float *)&D_L14_001615C8;
        s = *(float *)&D_L14_001615C4;
        if (x < s) {
            s = x;
            if (s < 0.0f) s = 0.0f;
        }
        func_L00_001FF4B0(v2, v1, s);
        func_001F9BD8(w, w, v2);
    }
    m[24] = 1.0f;
    g = *(int *)&D_L14_001615CC;
    *(int *)&m[16] = g;
    m[20] = 1.0f;
    m[21] = 1.0f;
    m[23] = 1.0f;
    m[22] = 0.0f;
    m[25] = 0.0f;
    m[26] = 0.0f;
    m[27] = 0.0f;
    *(int *)&m[19] = g;
    *(int *)&m[18] = g;
    *(int *)&m[17] = g;
    vp = m;
    tp = D_L14_001D8B90;
    for (i = 3; i >= 0; i--) {
        func_001F9C30(vp, tp, *(float *)&D_L14_001615C0);
        func_001F9BD8(vp, vp, w);
        tp += 16;
        vp += 4;
    }
    func_L00_001FD1D8(m, 0, 0);
}
