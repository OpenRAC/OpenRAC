/* NON_MATCHING func_L00_00269BE8 -- src/overlays/shared/partproc_002697A0.c
 * Best so far: SIZE ours 460 / retail 452, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Part update: kills part when out of range (func_L00_002688A8), else applies gravity (D_0015EE70*9.8) to vel an
 *   Left (p4.c, 460 vs 452 bytes): lui of D_L00_00160310 placed before the daddu $a0 instead of after; retail fill
 *   Would unblock: some wording that makes the L60 block begin with the gp float load so the scheduler copies it i
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_001F9BD8(void *, void *, void *);
extern float func_L00_002644E0(void *);
extern void func_L00_002688A8(void *);
extern float D_L00_00160310[];
extern float D_L00_00160320 MACRO_ADDR;
extern float D_L00_00160318 MACRO_ADDR;
extern short D_0015EE70_s __asm__("D_0015EE70");

// Updates a part: kills it when out of range, else applies gravity to its velocity.
void func_L00_00269BE8(char *p) {
    float *s = (float *)(p + 0x30);
    float lim = s[3];
    u128 v;
    u128 w;
    u128 x;
    int a, b;
    if (*(float *)(p + 0x18) < lim || *(float *)(p + 0x10) < 0.0f || *(float *)(p + 0x14) < 0.0f
        || 512.0f < *(float *)(p + 0x10) || 512.0f < *(float *)(p + 0x14)) {
        func_L00_002688A8(p);
        return;
    }
    a = (int)*(float *)(p + 0x20);
    b = (int)*(float *)(p + 0x24);
    func_001F9BD8(&v, s, D_L00_00160310);
    func_001F9BD8(p + 0x20, p + 0x20, &v);
    func_001F9BD8(p + 0x10, p + 0x10, &v);
    if (*(short *)(p + 0xA) != 2
        && (a != (int)*(float *)(p + 0x20) || b != (int)*(float *)(p + 0x24))) {
        float r;
        w = *(u128 *)(p + 0x20);
        r = func_L00_002644E0(&w);
        if (r < D_L00_00160320) {
            lim = D_L00_00160320;
        } else {
            x = *(u128 *)(p + 0x20);
            lim = func_L00_002644E0(&x);
        }
        if (*(float *)(p + 0x28) < lim) {
            func_L00_002688A8(p);
            return;
        }
    }
    s[2] = s[2] - 9.8f * *(float *)&D_0015EE70_s;
    s[3] = lim + D_L00_00160318;
}
