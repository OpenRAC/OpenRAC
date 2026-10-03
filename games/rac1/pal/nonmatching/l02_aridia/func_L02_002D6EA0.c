/* NON_MATCHING func_L02_002D6EA0 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: SIZE ours 536 / retail 540, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Particle burst spawn (random offsets, colour bytes) calling func_00219780. Best p3.c: only the v10->v20 128-bi
 */
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern void func_001F9BC0(void *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_00219780(void *, void *, void *, int, int, int, int, int, int);
extern float D_0015EE6C MACRO_ADDR;
extern int D_L02_001601F8 MACRO_ADDR;
extern short D_L02_001619F4, D_L02_001619F8, D_L02_001619FC, D_L02_00161A00;
extern short D_L02_00161A04, D_L02_00161A08, D_L02_00161A0C, D_L02_00161A10;
extern short D_L02_00161A14, D_L02_00161A18, D_L02_00161A1C, D_L02_00161A20;
extern short D_L02_00161A24, D_L02_00161A28, D_L02_00161A2C, D_L02_00161A30;
extern short D_L02_00161A34;
typedef int u128 __attribute__((mode(TI)));

/* Spawns a randomised particle burst at a position when the level counter allows. */
void func_L02_002D6EA0(char *pos) {
    float v10[4], v20[4], v30[4], v40[4];
    float *p10 = v10, *p20;
    float ang, c, s;
    int c0, c1, c2;
    *(u128 *)v10 = *(u128 *)pos;
    if (D_L02_001601F8 < 0x6A5) {
        ang = func_00214158();
        p20 = v20;
        *(u128 *)p20 = *(u128 *)p10;
        v20[2] += func_002140F8(0.125f, 0.0f);
        v20[0] += func_002140F8(-0.125f, 0.125f);
        v20[1] += func_002140F8(-0.125f, 0.125f);
        func_001F9BC0(v30);
        c = func_001F9F90(ang);
        v40[0] = c * (func_002140F8(*(float *)&D_L02_00161A04, *(float *)&D_L02_00161A08) * D_0015EE6C);
        s = func_001F9FA8(ang);
        v40[1] = s * (func_002140F8(*(float *)&D_L02_00161A04, *(float *)&D_L02_00161A08) * D_0015EE6C);
        v40[2] = 0.0f;
        v30[2] = func_002140F8(*(float *)&D_L02_001619F4, *(float *)&D_L02_001619F8) * D_0015EE6C;
        v40[2] = func_002140F8(*(float *)&D_L02_001619FC, *(float *)&D_L02_00161A00) * D_0015EE6C;
        v30[3] = *(float *)&D_L02_00161A0C;
        v40[3] = *(float *)&D_L02_00161A10;
        c0 = func_001FA898_r(func_002140F8((float)*(int *)&D_L02_00161A14, (float)*(int *)&D_L02_00161A18));
        c1 = func_001FA898_r(func_002140F8((float)*(int *)&D_L02_00161A1C, (float)*(int *)&D_L02_00161A20));
        c2 = func_001FA898_r(func_002140F8((float)*(int *)&D_L02_00161A24, (float)*(int *)&D_L02_00161A28));
        func_00219780(p20, v30, v40, *(int *)&D_L02_00161A2C, *(int *)&D_L02_00161A30, c0, c1, c2, *(int *)&D_L02_00161A34);
    }
}
