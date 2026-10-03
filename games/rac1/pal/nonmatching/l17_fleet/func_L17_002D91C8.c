/* NON_MATCHING func_L17_002D91C8 -- src/overlays/l17_fleet/vendor_002AA068.c
 * Best so far: SIZE ours 784 / retail 792, checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-03): match its declarations to the file's first.
 * What the last attempts found:
 *   Draws two passes of 3 rings of 4 vertices around a moby: builds a 4-point 2D table (D_L17_001D4240 + s*e[o]), 
 *   Best is p4.c (size 792 matches; structure, loops, compound-literal rot, struct copies, MACRO_ADDR for D_L17_00
 *   Swapping rot/e statement order in the inner loop did not move it; would need a source shape that changes the i
 *   [l17n3 u03] p5 = p4 minus the u128 typedef (the file now declares it earlier; p4 no longer compiled). p6 (inne
 *   Left: retail hoists the loop-invariant addresses in order &rot(0xE0), &e(0xD0), &pts.x, &pts.y (ours &e first)
 *   p7 (declare rot before e) swaps the stack slots (e at 0xE0): wrong. p8 (x store before rot) worse (227). Budge
 *   [l17n4 v01] Fresh budget used (p10-p15). packet best.c + MACRO_ADDR on D_L17_0015F6B0 (p10) fixed the first di
 *   Still left: hoisted-invariant order. Retail hoists &rot(0xE0 ->0x10C), &e(0xD0 ->0x104), &pts.x, &pts.y into a
 */
typedef int u128_2D91C8 __attribute__((mode(TI)));
typedef struct { float v[2]; } V2;
typedef struct { float v[4]; } V4;
extern float func_001FA888(int);
extern float func_L00_00200210(float, float);
extern void func_001FA218(float *, float *);
extern int func_001F4868(int);
extern float func_001FA7D8(float x);
extern void func_00215C00(void *, float, float, float);
extern void func_L00_001FD1D8(void *, void *, int);
extern int D_L17_0015F6B0 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_L17_001D4240[];
extern V2 D_L17_00161C40;
extern V2 D_L17_00161C48;
extern short D_L17_00161C3C;
extern short D_L17_00161C1C;
extern short D_L17_00161C18;
extern short D_L17_00161C20;
extern short D_L17_00161C24;
extern short D_L17_00161C00;
extern short D_L17_00161C04;
extern short D_L17_00161BFC;
extern short D_L17_00161C38;

/* draws a spinning ring of four vertices three times over, twice, around a moby */
void func_L17_002D91C8(char *moby) {
    float vec[5][4];
    float pts[8];
    long gs[4];
    float mat[16];
    V2 e;
    V4 rot;
    float s;
    int o;
    int i;
    int j;
    s = func_001FA888(D_L17_0015F6B0) * (*(float *)&D_L17_00161C3C * D_0015EE6C);
    s = func_L00_00200210(s, 1.0f);
    func_001FA218(mat, (float *)(moby + 0x40));
    *(u128_2D91C8 *)&mat[12] = *(u128_2D91C8 *)(moby + 0x10);
    mat[15] = 1.0f;
    gs[1] = func_001F4868(0x28);
    gs[2] = 0xFF9000000260L;
    gs[3] = ((long)*(int *)&D_L17_00161C1C << 2) | (long)*(int *)&D_L17_00161C18 | ((long)*(int *)&D_L17_00161C20 << 4) | ((long)*(int *)&D_L17_00161C24 << 6) | 0x8000000000L;
    gs[0] = 0;
    for (o = 0; o < 2; o++) {
        for (j = 0; j < 4; j++) {
            e = D_L17_00161C40;
            pts[j * 2 + 1] = D_L17_001D4240[j * 2 + 1] + s * e.v[o];
        }
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 4; j++) {
                float r;
                float *vj = vec[j];
                e = D_L17_00161C48;
                rot = (V4){{0.0f, -*(float *)&D_L17_00161C00 * 0.017453292f, *(float *)&D_L17_00161C00 * 0.017453292f, 0.0f}};
                pts[j * 2] = D_L17_001D4240[j * 2] + s * e.v[o];
                r = 0.0f;
                if (j != 0)
                    r = *(float *)&D_L17_00161C04;
                func_00215C00(vj, r, 1.5707964f, func_001FA7D8(*(float *)&D_L17_00161BFC * 0.017453292f + func_001FA888(i) * 2.0943952f + rot.v[j]));
                vj[3] = 1.0f;
            }
            *(int *)&vec[4][0] = *(int *)&D_L17_00161C38;
            *(int *)&vec[4][1] = *(int *)&D_L17_00161C38;
            *(int *)&vec[4][2] = *(int *)&D_L17_00161C38;
            *(int *)&vec[4][3] = *(int *)&D_L17_00161C38;
            func_L00_001FD1D8(vec, mat, 0);
        }
    }
}
