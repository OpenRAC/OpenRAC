/* NON_MATCHING func_L14_002B4668 -- src/overlays/shared/vendor_002B2A28.c
 * Best so far: BYTES 13/352 (96.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L14_002B4668: steers a moby: queries a path (func_L00_0025E860), eases x/y position via two func_L00_0025
 *   p1/p2/p3 (same bytes) are SIZE-correct, BYTES 13/352: the whole function matches except the instruction order 
 *   Wordings tried: angle as local, k = D70*4pi as local or inline, m = D6C*2pi as local or inline. Would unblock:
 */
extern int func_L00_0025E860(void *, void *, void *, void *, int, float);
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern void func_001F9BF0(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_L00_0025CE58(float *p, float *v, float a, float b, float c, float d);
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern short D_L14_001615A0;

/* steers the moby toward its target: queries the path, eases x/y, then turns to face the heading */
int func_L14_002B4668(char *moby) {
    float a[4];
    float b[4];
    float c[4];
    char *data = *(char **)(moby + 0x78);
    int r = func_L00_0025E860(D_L14_001B0F30[*(int *)(data + 0x240)], a, data + 0x24C, data + 0x250, 0,
                              *(float *)&D_L14_001615A0 * D_0015EE6C * *(float *)(data + 0x27C));
    float ang;
    float k;
    float m;
    qcopy(b, moby + 0x10);
    func_L00_0025C918((float *)(moby + 0x10), (float *)(data + 0x254), a[0], 0.01f, 0.2f, 0.0f);
    func_L00_0025C918((float *)(moby + 0x14), (float *)(data + 0x258), a[1], 0.01f, 0.2f, 0.0f);
    func_001F9BF0(c, a, b);
    ang = func_L00_001FF860(c[0], c[1]);
    k = D_0015EE70 * 12.566371f;
    m = D_0015EE6C * 6.2831855f;
    func_L00_0025CE58((float *)(moby + 0x48), (float *)(data + 0x23C), ang, k, k, m);
    return r != 0;
}
