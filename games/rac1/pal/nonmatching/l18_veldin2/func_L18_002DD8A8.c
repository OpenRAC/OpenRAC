/* NON_MATCHING func_L18_002DD8A8 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 74/1116 (93.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Best remains p6.c (BYTES 74/1116; p5.c 75). Spill-slot reading: retail slots (high to low) ang(0x2D4), rot0, u
 *   Tried (all runs with --diff):
 *   - p10: t/gi/r computed right after the tex call: 159 (worse).
 *   - p11/p12: gi folded into r / gi placed before t: output identical to p6 (source order of gi does not move the
 *   - p13/p14: `V_dd8a8 *av = &rot[3]` pointer local created before the loop (to make its pseudo older): 240 / 221
 *   Remaining diff: the 2C4/2D4 slot swap (+ 3 uses, 0x25c,0x334,0x350) and gp5C load+mul order at 0x108-0x154. Al
 *   ## Round 3 (wave 6 retry, 4 runs)
 *   Still p6.c (BYTES 74/1116). p15 (rot declared before ang): identical to p6. p16 (ang/rot block-scope locals af
 */
typedef struct { float f[4]; } __attribute__((aligned(16))) V_dd8a8;
typedef struct { float u, v; } UV_dd8a8;
typedef struct {
    V_dd8a8 corner[4];
    unsigned int color[4];
    UV_dd8a8 uv[4];
    long unk70, tex, unk80, unk88;
} Q_dd8a8;
extern void func_001FA190(void *);
extern int func_001F4868(int);
extern void func_L00_001FD1D8(void *, void *, int);
extern void func_001FA4F0(void *, void *, void *);
extern float func_001FA888(int);
extern int func_001F9850(int);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_001FA8A8(int, int, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9BC0(void *);
extern void func_001FA1F8(void *, void *);
extern float D_L18_001D4600[];
extern short D_L18_00161D2C;
extern short D_L18_00161D34;
extern short D_L18_00161D38;
extern short D_L18_00161D3C;
extern short D_L18_00161D40;
extern short D_L18_00161D48;
extern short D_L18_00161D4C;
extern short D_L18_00161D50;
extern short D_L18_00161D54;
extern short D_L18_00161D58;
extern short D_L18_00161D5C;
extern short D_L18_00161D64;

void func_L18_002DD8A8_impl(char *moby) __asm__("func_L18_002DD8A8");
void func_L18_002DD8A8_impl(char *moby) {
    Q_dd8a8 quad[4];
    V_dd8a8 mat[4];
    V_dd8a8 ang;
    V_dd8a8 rot[3];
    float *p;
    int i, j, k;
    int c;
    float t, r, d, s, gi;

    p = *(float **)(moby + 0x78);
    func_001FA190(mat);
    qcopy(&mat[3], moby + 0x10);
    mat[3].f[2] = mat[3].f[2] + *(float *)&D_L18_00161D54;
    mat[3].f[3] = 1.0f;
    for (i = 0; i < 4; i++) {
        quad[i].tex = func_001F4868(14);
        gi = *(float *)&D_L18_00161D5C * (float)i;
        quad[i].unk70 = 0;
        quad[i].unk80 = 0xFF9000000260L;
        quad[i].unk88 = (long)*(int *)&D_L18_00161D34 | ((long)*(int *)&D_L18_00161D38 << 2) |
                        ((long)*(int *)&D_L18_00161D3C << 4) | ((long)*(int *)&D_L18_00161D40 << 6) | 0x8000000000L;
        t = p[1] + (float)i * 0.25f;
        r = p[0] + gi;
        t = t - (float)func_001FA898_r(t);
        c = func_001FA8A8(*(int *)&D_L18_00161D48, *(int *)&D_L18_00161D4C, t);
        s = t * *(float *)&D_L18_00161D64;
        if (s > 1.0f) s = 1.0f;
        c = func_001FA8A8(c & 0xFFFFFF, c, s);
        d = func_001FA888(((int *)p)[2]);
        s = d / (float)func_001F9850(*(int *)&D_L18_00161D2C);
        if (s > 1.0f) {
            s = 1.0f;
        } else if (s < 0.0f) {
            s = 0.0f;
        }
        c = func_001FA8A8(c & 0xFFFFFF, c, s);
        for (j = 0; j < 4; j++) {
            float a;
            quad[i].uv[j].u = D_L18_001D4600[j * 2];
            quad[i].uv[j].v = D_L18_001D4600[j * 2 + 1];
            if (j & 1) {
                quad[i].corner[j].f[2] = t * *(float *)&D_L18_00161D58;
            } else {
                quad[i].corner[j].f[2] = -t * *(float *)&D_L18_00161D58;
            }
            a = func_001FA748((float)(j >> 1) * (*(float *)&D_L18_00161D50 * 0.017453292f) - 3.14159f,
                              *(float *)&D_L18_00161D50 * 0.25f * (float)i * 0.017453292f);
            quad[i].corner[j].f[0] = func_001F9F90(a) * r;
            quad[i].corner[j].f[1] = func_001F9FA8(a) * r;
            quad[i].corner[j].f[3] = 1.0f;
            quad[i].color[j] = c;
        }
    }
    func_001F9BC0(&ang);
    ang.f[2] = *(float *)&D_L18_00161D50 * 0.017453292f;
    func_001FA1F8(&rot[0], &ang);
    for (k = 0; (float)k < 360.0f / *(float *)&D_L18_00161D50; k++) {
        func_L00_001FD1D8(&quad[0], mat, 0);
        func_L00_001FD1D8(&quad[1], mat, 0);
        func_L00_001FD1D8(&quad[2], mat, 0);
        func_L00_001FD1D8(&quad[3], mat, 0);
        func_001FA4F0(mat, mat, &rot[0]);
    }
}
