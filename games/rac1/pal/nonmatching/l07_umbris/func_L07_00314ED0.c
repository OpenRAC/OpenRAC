/* NON_MATCHING func_L07_00314ED0 -- src/overlays/l07_umbris/vendor_00313D28.c
 * Best so far: BYTES 21/568 (96.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Steer a moby toward a heading (or contact-handle a pickup), returns state flags. Best p5.c (21 bytes differ, s
 */
extern unsigned char D_0013F450[] NOT_SDA;
extern char *D_L07_001B0830[];
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_L07_0015F6B0 MACRO_ADDR;
extern int D_L07_00173F40[];
extern float func_001F9D48(void *, void *);
extern int func_L00_0025A060(char *m, char *p, float *pos, char *v);
extern int func_L01_0028C2D8(void *, void *, float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF548(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern void func_L00_002592B0(char *moby, float *vel, float target, float k, float d, float max);
extern int func_L00_00259428(void *p, int x, int y, float f, float g);
extern void func_001F9C30(void *, void *, float);

/* steers a moby toward a target, then handles the pickup contact */
int func_L07_00314ED0(char *m, char *a, void *p6, float *p7, float thr) {
    float v[4];
    char *mp = m + 0x10;
    float d = func_001F9D48(mp, p6);
    int r = 4;
    char *tbl = D_L07_001B0830[*(int *)(a + 0x290)];
    if (thr < d) {
        r = func_L00_0025A060(m, a + 0x70, p7, a + 0x100);
        if (r & 2) {
            int i = func_L01_0028C2D8(mp, tbl, 0.0f);
            func_001F9BF0(v, tbl + (i * 16 + 0x10), mp);
            func_L00_001FF548(v, v, *(float *)(a + 0x94));
            func_001F9BD8(mp, mp, v);
        }
    } else {
        char *g = D_0013F450;
        float h;
        float dx = *(float *)(g + 0x80) - *(float *)(m + 0x10);
        float dy = *(float *)(g + 0x84) - *(float *)(m + 0x14);
        h = func_L00_001FF860(dx, dy);
        func_L00_002592B0(m, (float *)(a + 0x168), h, D_0015EE70 * 2.0943952f, D_0015EE70 * 0.6981317f, D_0015EE6C * 2.0943952f);
    }
    if ((D_L07_0015F6B0 & 3) == 3) {
        char *mp2 = m + 0x10;
        if (func_L00_00259428(mp2, (int)m, 0, 0.1f, 1.0f)) {
            int *t = D_L07_00173F40;
            unsigned char *q = *(unsigned char **)(t + 6);
            if (q != 0 && t[7] > 0 && *(short *)(q + 0xA6) == 0x42D && q[0xBC] == 0) {
                if (*(unsigned char *)(*(char **)(q + 0x78) + 0xBE) != 0) {
                    func_001F9BF0(v, q + 0x10, mp2);
                    v[2] = 0;
                    func_001F9C30(v, v, 0.5f);
                    func_001F9BD8(mp2, mp2, v);
                }
                (*(unsigned char **)(t + 6))[0xBC] = 1;
            }
        }
    }
    return r;
}
