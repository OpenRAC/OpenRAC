/* NON_MATCHING func_L15_002EC080 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: SIZE ours 1024 / retail 1040, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   func_L15_002EC080 builds three ring meshes around a moby (vertices, colours, 2D offsets from a rotating table)
 *   Prologue, the four func_00234C98 calls and the start match word for word (p2.c). The difference is loop streng
 *   Would unblock: knowing what stops gcc's loop.c reducing the giv (lifetime x benefit vs insn count), or a per-f
 */
extern float func_001FA888(int);
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern float func_001F9878(float);
extern float func_001FA748(float, float);
extern void func_L00_00251358(void *, void *, void *, void *);
extern float func_001F9B50(float);
extern float func_L00_00200210(float, float);
extern float func_001FA7D8(float x);
extern float func_001F9FA8(float);
extern void func_001F7868(void);
extern void func_L00_001FDE48(int, int, int, void *, int);
extern int D_L15_0015F6B0 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L15_00162100;
extern short D_L15_00162110;
extern short D_L15_00162120;
extern short D_L15_00162130;
extern short D_L15_00162140;
extern short D_L15_00162150;
extern short D_L15_0016216C;
extern short D_L15_00162170;
extern short D_L15_00162174;
extern short D_L15_00162178;
extern short D_L15_0016217C;
extern short D_L15_00162180;
extern short D_L15_00162184;
struct V4 { float f[4]; };
extern struct V4 D_L15_00162188;

#define CNT ((int *)&D_L15_00162100)
#define P110 ((char **)&D_L15_00162110)
#define P120 ((char **)&D_L15_00162120)
#define P130 ((char **)&D_L15_00162130)
#define P140 ((char **)&D_L15_00162140)
#define P150 ((char **)&D_L15_00162150)

/* Builds the three ring meshes of a moby's effect and submits them. */
void func_L15_002EC080(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float s;
    int r, g, b, col;
    int i, j, k;
    struct V4 t;
    s = func_001FA888(D_L15_0015F6B0) * (*(float *)&D_L15_00162184 * D_0015EE6C);
    func_00234C98(6, func_001F4868(*(int *)&D_L15_00162170));
    func_00234C98(0x42, ((long)*(int *)&D_L15_0016216C << 32) | 0x44);
    func_00234C98(8, 0);
    func_00234C98(0x14, (0xFF90L << 32) | 0x260);
    *(float *)(data + 0) = *(float *)(data + 4);
    *(float *)(data + 4) = func_001FA748(*(float *)(data + 4), 360.0f / func_001F9878(*(float *)&D_L15_0016217C) * 0.017453292f * D_0015EE6C);
    func_L00_00251358(moby, &r, &g, &b);
    col = (*(int *)&D_L15_00162174 << 24) | (b << 16) | (g << 8) | r;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < CNT[i]; j++) {
            float *v = (float *)(P140[i] + j * 12);
            float c;
            float len = func_001F9B50(v[0] * v[0] + v[1] * v[1]);
            float a = func_L00_00200210(len, *(float *)&D_L15_00162178);
            c = func_001F9FA8(func_001FA748(func_001FA7D8(a * 6.2831855f / *(float *)&D_L15_00162178), *(float *)(data + 4)));
            *(float *)(P110[i] + j * 12) = v[0] + *(float *)(moby + 0x10);
            *(float *)(P110[i] + j * 12 + 4) = v[1] + *(float *)(moby + 0x14);
            *(float *)(P110[i] + j * 12 + 8) = v[2] + *(float *)(moby + 0x18) + *(float *)&D_L15_00162180 * c;
            *(int *)(P130[i] + j * 4) = col;
        }
    }
    k = 0;
    do {
        for (i = 0; i < 3; i++) {
            for (j = 0; j < CNT[i]; j++) {
                float *q = (float *)(P150[i] + j * 8);
                float *o = (float *)(P120[i] + j * 8);
                t = D_L15_00162188;
                o[0] = q[0] + func_L00_00200210(s * t.f[k * 2], 1.0f);
                o[1] = q[1] + func_L00_00200210(s * t.f[k * 2 + 1], 1.0f);
            }
        }
        k++;
    } while (k < 2);
    func_001F7868();
    for (i = 0; i < 3; i++)
        func_L00_001FDE48(CNT[i], (int)P110[i], (int)P130[i], P120[i], 1);
}
