/* NON_MATCHING func_L15_002EF030 -- src/overlays/l15_quartu/vendor_002EDB50.c
 * Best so far: SIZE ours 880 / retail 888, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L15_002EF030: per-frame ring-of-points builder for a moby (two loops of 46 entries over D_L15_001DEB30/00
 *   Remaining: 2 instructions short, and the retail loop-1 base kept in its own register ($22 base, $19 copy, $30 
 */
typedef struct { float f[4]; } V4;
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
extern float D_L15_001DEF80[46][3];
extern int D_L15_001DF318[46];
extern float D_L15_001DEB30[46][3];
extern float D_L15_001DED58[46][2];
extern float D_L15_001DF1A8[46][2];
extern V4 D_L15_00162288;
extern short D_L15_0016227C;
extern short D_L15_00162268;
extern short D_L15_00162264;
extern short D_L15_00162274;
extern short D_L15_0016226C;
extern short D_L15_00162270;
extern short D_L15_00162280;
extern short D_L15_00162278;
extern short D_L15_00162258;
extern short D_L15_00162260;
extern short D_L15_00162250;
extern short D_L15_00162230;
/* Draws the animated ring of points around a moby: computes the vertex and colour tables, then submits each pass. */
void func_L15_002EF030(char *moby) {
    float *data = *(float **)(moby + 0x78);
    float s;
    V4 uv;
    int c[4];
    int i;
    unsigned j, n;
    float *in, *po, *ob;
    int off;
    float *w;

    s = func_001FA888(D_L15_0015F6B0) * (*(float *)&D_L15_0016227C * D_0015EE6C);
    func_00234C98(6, func_001F4868(*(int *)&D_L15_00162268));
    func_00234C98(0x42, ((long)*(int *)&D_L15_00162264 << 32) | 0x44);
    func_00234C98(8, 0);
    func_00234C98(0x14, 0xFF9000000260L);
    data[0] = data[1];
    data[1] = func_001FA748(data[1], 360.0f / func_001F9878(*(float *)&D_L15_00162274) * 0.017453292f * D_0015EE6C);
    func_L00_00251358(moby, &c[0], &c[1], &c[2]);
    w = &uv.f[1];
    c[3] = (*(int *)&D_L15_0016226C << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
    for (n = 0, in = D_L15_001DEB30[0], ob = D_L15_001DEF80[0], po = ob, off = 0; n < 46; n++, in += 3, po += 3, off += 12) {
        float a, b, e, d;
        d = func_001F9B50(in[0] * in[0] + in[1] * in[1]);
        a = func_L00_00200210(d, *(float *)&D_L15_00162270);
        b = func_001FA7D8(a * 6.2831855f / *(float *)&D_L15_00162270);
        e = func_001F9FA8(func_001FA748(b, data[1]));
        po[0] = *(float *)&D_L15_00162280 * in[0] + *(float *)(moby + 0x10);
        ((float *)((char *)ob + off))[1] = *(float *)&D_L15_00162280 * in[1] + *(float *)(moby + 0x14);
        D_L15_001DEF80[n][2] = *(float *)&D_L15_00162280 * in[2] + *(float *)(moby + 0x18) + *(float *)&D_L15_00162278 * e;
        D_L15_001DF318[n] = c[3];
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 46; j++) {
            uv = D_L15_00162288;
            D_L15_001DF1A8[j][0] = D_L15_001DED58[j][0] + func_L00_00200210(s * uv.f[i * 2], 1.0f);
            D_L15_001DF1A8[j][1] = D_L15_001DED58[j][1] + func_L00_00200210(s * w[i * 2], 1.0f);
        }
        func_001F7868();
        for (j = 0; j < 1; j++) {
            func_L00_001FDE48(((int *)&D_L15_00162230)[j], ((int *)&D_L15_00162250)[j], ((int *)&D_L15_00162260)[j], (void *)((int *)&D_L15_00162258)[j], 1);
        }
    }
}
