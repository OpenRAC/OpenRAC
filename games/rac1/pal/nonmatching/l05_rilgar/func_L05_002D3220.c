/* NON_MATCHING func_L05_002D3220 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: BYTES 16/508 (96.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a burst of 16 particles then a ring of 16 sparks around a position. Best is p3.c (16 bytes differ, same
 *   Only the placement of the hoisted address of D_L05_001613A8: ours does lui $v0 early then addiu $s3; retail do
 *   Local-pointer wordings (statement, for-init) gave the same bytes; a scheduling tie.
 */
extern char *func_L00_002D9340(void *, float);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_L00_00258BC8(int, int);
extern int func_002140B0(int);
extern void func_L00_002703E8(void *, void *, int, int);
extern void func_L00_00258DB0(float *, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern unsigned char *func_L00_00272770(void *, void *, void *, float, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern char D_L05_001613A8[];
extern float D_L05_001613A8_m __asm__("D_L05_001613A8") MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char D_L05_0015F660[] MACRO_ADDR;

/* Spawns a burst of particles and a ring of sparks around a position. */
void func_L05_002D3220(int unused, char *p) {
    float v[4];
    char *r;
    int i;
    char *base;
    int n;
    r = func_L00_002D9340(p, 3.0f);
    if (r) r[0x23] = 0x70;
    for (base = D_L05_001613A8, i = 15; i >= 0; i--) {
        float ang = func_00214158();
        float k = func_002140F8(D_0015EE6C * 0.0f, D_0015EE6C * 3.0f);
        v[0] = func_001F9F90(ang) * k;
        v[1] = func_001F9FA8(ang) * k;
        v[2] = func_002140F8(D_0015EE6C * 3.0f, D_0015EE6C * 6.5f);
        n = func_L00_00258BC8(0x5A, 0x78);
        func_L00_002703E8(p, v, func_002140B0(2), n);
    }
    for (i = 0; i < 16; i++) {
        unsigned char *q;
        func_L00_00258DB0(v, 1.0f, 1.0f);
        func_001F9BD8(v, v, p);
        v[2] = D_L05_001613A8_m + 0.05f;
        q = func_L00_00272770(v, D_L05_0015F660, base, func_002140F8(0.7f, 1.0f), i == 0 ? 2.0f : -2.0f);
        if (q) {
            *(short *)(q + 0xA) = func_001FA898_r(func_001F9878(func_002140F8(30.0f, 60.0f)));
        }
    }
}
