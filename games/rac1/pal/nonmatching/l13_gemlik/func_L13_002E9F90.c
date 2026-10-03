/* NON_MATCHING func_L13_002E9F90 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: BYTES 34/460 (92.6% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Loops over the -1 terminated list D_L13_001D3808 spawning particles (func_L00_0026A7F8) per entry; best is p4.
 *   Left: prologue register choice for the list head (retail lui $a1, lw $v1, -1 in $v0; ours lui $v0, -1 in $v1, 
 *   A pointer local to the zero vector (u128 zv; u128 *z=&zv, declared before p) gets the hoisted &z in s3; unbloc
 */
extern int D_L13_001D3808[];
extern int D_L13_0015F6B0 MACRO_ADDR;
extern float func_002140F8(float, float);
extern void func_L00_00250800(void *, int, void *);
extern void func_L00_00260958(float *v, float s);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_0026A7F8(void *, void *, int, int, int, int, int, int);
extern int func_002140B0(int);
typedef int u128 __attribute__((mode(TI)));

// Spawns a burst of particles for each entry of the level's effect list.
void func_L13_002E9F90(char *moby) {
    u128 zv;
    u128 *z = &zv;
    float a[4];
    int *p;
    for (p = D_L13_001D3808; *p != -1; p++) {
        float s;
        int r, k;
        zv = 0;
        s = *(float *)(moby + 0x2C) / (*(float *)(*(char **)(moby + 0x24) + 0x24) * 4.0f);
        s = s * func_002140F8(0.8f, 1.1f);
        func_L00_00250800(moby, *p, a);
        func_L00_00260958(a, s * 1.4f);
        if (D_L13_0015F6B0 & 1) {
            r = func_001F9850(func_L00_00258BC8(0x14, 0x3C));
            k = func_001FA898_r(func_002140F8(450.0f, 550.0f) * s);
            func_L00_0026A7F8(a, z, 0xDF000FFF, 0xCF, r, k, -100, 1);
        } else {
            if (*(unsigned char *)(moby + 0xBC) != 0) {
                if (func_002140B0(0xC) != 0) continue;
            }
            r = func_001F9850(func_L00_00258BC8(0x14, 0x32));
            k = func_001FA898_r(func_002140F8(600.0f, 800.0f) * s);
            func_L00_0026A7F8(a, z, 0x8000FFFF, 0x80, r, k, -100, 1);
        }
    }
}
