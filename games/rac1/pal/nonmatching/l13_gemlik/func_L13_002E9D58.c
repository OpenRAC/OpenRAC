/* NON_MATCHING func_L13_002E9D58 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: SIZE ours 556 / retail 568, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws three reticle arcs (func_L00_0026A7F8) for each of six targets, scaled by moby+0x2C over the model scale
 *   Our best (p1/p2/p3, identical bytes) is 12 bytes short: retail keeps a ninth saved register ($s3, a copy of &l
 *   Unblock: some source form that keeps a second copy of the local's address live across the calls.
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_L00_00250800(void *, int, void *);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_0026A7F8(void *, void *, int, int, int, int, int, int);
extern int func_002140B0(int);
extern float func_002140F8(float, float);
extern int D_L13_001D37D8[];
extern int D_L13_001D37F0[];
extern int D_L13_001D3748[];
extern int D_L13_0015F6B0 MACRO_ADDR;

/* Draws the lock-on reticle arcs for each of six targets. */
void func_L13_002E9D58(char *moby, char *tbl) {
    float v[8];
    float *w = v + 4;
    int i;
    int *pa = D_L13_001D37D8;
    int *pb = D_L13_001D37F0;
    int *pc = D_L13_001D3748;
    float f;
    int s, r;
    for (i = 5; i >= 0; i--) {
        if (*(int *)(tbl + 0xC4 + *pa * 4) != 0) {
            *(u128 *)w = 0;
            f = *(float *)(moby + 0x2C) / (*(float *)(*(char **)(moby + 0x24) + 0x24) * 4.0f);
            func_L00_00250800(moby, *pb, v);
            if (D_L13_0015F6B0 & 1) {
                s = func_001F9850(func_L00_00258BC8(0xF, 0x14));
                r = func_001FA898_r(f * 351.0f);
                func_L00_0026A7F8(v, w, 0xCF0000FF, 0xCF, s, r, -100, 1);
            } else {
                if (((unsigned char *)moby)[0xBC] != 0 && pc[((unsigned char *)moby)[0xBC] - 1] == *pa && func_002140B0(9)) {
                } else {
                    s = func_001F9850(func_L00_00258BC8(0xF, 0x16));
                    r = func_001FA898_r(f * 600.0f);
                    func_L00_0026A7F8(v, w, 0x6000FFFF, 0x80, s, r, -100, 1);
                }
            }
            r = func_001FA898_r(func_002140F8(100.0f, 250.0f) * f);
            s = func_001F9850(func_L00_00258BC8(8, 0xC));
            func_L00_0026A7F8(v, w, 0xEFFF7F4F, 0xFF0000, s, r, -r, 1);
        }
        pb++;
        pa++;
    }
}
