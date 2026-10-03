/* NON_MATCHING func_L02_002D6C30 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: SIZE ours 628 / retail 620, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns particle burst at pos (a0), count a1 scaled by detail D_L02_001601F8 (>=0x5DD: /4, >0x400: /2); loop ca
 *   Stopped at budget: remaining diff is pointer-hoist/register allocation (retail hoists &v (s5=sp+0x10), &w (s4)
 */
typedef int u128 __attribute__((mode(TI)));
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern void func_001F9BC0(float *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);
extern float D_0015EE6C MACRO_ADDR;
extern int D_L02_001601F8;
extern short D_L02_001619B0, D_L02_001619B4, D_L02_001619B8, D_L02_001619BC;
extern short D_L02_001619C0, D_L02_001619C4, D_L02_001619C8, D_L02_001619CC;
extern short D_L02_001619D0, D_L02_001619D4, D_L02_001619D8, D_L02_001619DC;
extern short D_L02_001619E0, D_L02_001619E4, D_L02_001619E8, D_L02_001619EC;
extern short D_L02_001619F0;

// Spawns a burst of particles around a position; count scales with detail level.
void func_L02_002D6C30(void *pos, int n) {
    float v[4];
    float w[4];
    float v3[4];
    float v4[4];
    float r;
    int a, b, c, cnt;
    float *pv = v;
    *(u128 *)v = *(u128 *)pos;
    if (D_L02_001601F8 >= 0x5DD) {
        n = n / 4;
    } else if (D_L02_001601F8 > 0x400) {
        n = n / 2;
    }
    if (n > 0) {
        float *pw = w, *p3 = v3;
        cnt = n;
        do {
            r = func_00214158();
            *(u128 *)pw = *(u128 *)pv;
            w[2] = w[2] + func_002140F8(-0.25f, -0.75f);
            w[0] = w[0] + func_002140F8(-0.25f, 0.25f);
            w[1] = w[1] + func_002140F8(-0.25f, 0.25f);
            func_001F9BC0(p3);
            v4[0] = func_001F9F90(r) * (func_002140F8(*(float *)&D_L02_001619C0, *(float *)&D_L02_001619C4) * D_0015EE6C);
            v4[1] = func_001F9FA8(r) * (func_002140F8(*(float *)&D_L02_001619C0, *(float *)&D_L02_001619C4) * D_0015EE6C);
            *(int *)&v4[2] = 0;
            v3[2] = func_002140F8(*(float *)&D_L02_001619B0, *(float *)&D_L02_001619B4) * D_0015EE6C;
            v4[2] = func_002140F8(*(float *)&D_L02_001619B8, *(float *)&D_L02_001619BC) * D_0015EE6C;
            v3[3] = *(float *)&D_L02_001619C8;
            v4[3] = *(float *)&D_L02_001619CC;
            a = func_001FA898_r(func_002140F8((float)*(int *)&D_L02_001619D0, (float)*(int *)&D_L02_001619D4));
            b = func_001FA898_r(func_002140F8((float)*(int *)&D_L02_001619D8, (float)*(int *)&D_L02_001619DC));
            c = func_001FA898_r(func_002140F8((float)*(int *)&D_L02_001619E0, (float)*(int *)&D_L02_001619E4));
            func_00219780(pw, p3, v4, *(int *)&D_L02_001619E8, *(int *)&D_L02_001619EC, a, b, c, *(int *)&D_L02_001619F0);
        } while (--cnt);
    }
}
