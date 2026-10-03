/* NON_MATCHING func_L00_0026D700 -- src/overlays/shared/partupd_0026A130.c
 * Best so far: SIZE ours 856 / retail 844, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Type-20 particle update: orbits a matrix, fades, kills itself on contact or timeout. Best p4.c (860 vs 844 byt
 *   stack layout and call sequence match. Left: the D_0015EE60 read at the start of the shared tail is a $gp lwc1 
 *   delay slots of both branches in retail (and the later 1<=p[1] block reads it via $gp); ours uses lui form and 
 *   Declaring a short alias (D_0015EE60_g) did not produce $gp here (assembler still emits lui). Also store of p[5
 *   after func_001FA748 and an extra saved reg. Unblock: a way to get the $gp form for this symbol next to MACRO_A
 */
extern float func_001F9FA8(float);
extern float func_001F9F90(float);
extern void func_001F9EE8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_00250800(void *, int, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001FA748(float, float);
extern float func_L00_0025D288(void *a, void *b, void *c, int d);
extern int func_001F9938(void *);
extern void func_L00_002688A8(void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float D_0015EE60 MACRO_ADDR;
typedef int xu128 __attribute__((mode(TI)));
typedef struct { xu128 r[3]; } Mx;
typedef struct { float x, y, z, w; } __attribute__((aligned(16))) Vx;

// Per-frame update of a type-20 particle: orbits around a matrix, fades, and kills itself on contact or timeout.
void func_L00_0026D700(char *m) {
    char *p = m + 0x20;
    char *pos = m + 0x10;
    if (*(float *)(p + 0x10) != 0.0f) {
        Mx mat;
        Vx u, v, w;
        char *h;
        float d;
        mat = **(Mx **)(m + 0x20);
        v.y = func_001F9FA8(*(float *)(p + 0x14)) * *(float *)(p + 0x10);
        v.z = func_001F9F90(*(float *)(p + 0x14)) * *(float *)(p + 0x10);
        v.x = 0.0f;
        v.w = 0.0f;
        func_001F9EE8(&w, &v, &mat);
        func_001F9BF0(pos, pos, &w);
        *(float *)(p + 0x1C) = *(float *)(p + 0x1C) + D_0015EE60 * 0.06001f;
        func_L00_00250800(*(void **)(p + 0x18), 0, &u);
        func_001F9BF0(&v, &u, pos);
        func_L00_001FF4B0(&v, &v, *(float *)(p + 0x1C));
        func_001F9BD8(pos, pos, &v);
        *(float *)(p + 0x14) = func_001FA748(*(float *)(p + 0x14), D_0015EE60 * 0.5235988f);
        *(float *)(p + 0x10) = *(float *)(p + 0x10) * (D_0015EE60 * -0.1f + 1.0f);
        v.y = func_001F9FA8(*(float *)(p + 0x14)) * *(float *)(p + 0x10);
        v.z = func_001F9F90(*(float *)(p + 0x14)) * *(float *)(p + 0x10);
        v.x = 0.0f;
        v.w = 0.0f;
        func_001F9EE8(&w, &v, &mat);
        func_001F9BD8(pos, pos, &w);
        d = func_L00_0025D288(&u, *(char **)(p + 0x18) + 0xC0, pos, 1);
        if (0.0f < d) {
            func_L00_002688A8(m);
            return;
        }
    }
    *(float *)(p + 4) = *(float *)(p + 4) + D_0015EE60 * 0.011f * *(float *)(p + 0xC);
    if (*(float *)(p + 4) <= 0.0f || func_001F9938(m + 0xA)) {
        func_L00_002688A8(m);
        return;
    }
    if (1.0f <= *(float *)(p + 4)) {
        *(float *)(p + 0xC) = -*(float *)(p + 0xC);
        *(float *)(p + 4) = *(float *)(p + 4) + D_0015EE60 * 0.011f * *(float *)(p + 0xC);
    }
    *(float *)(m + 0xC) = *(float *)(m + 0xC) + D_0015EE60 * -2940.0f;
    *(int *)(m + 4) = (*(int *)(m + 4) & 0xFFFFFF) | (func_001FA898_r(*(float *)(p + 4) * 255.0f) << 24);
    *(float *)(p + 8) = *(float *)(p + 8) + D_0015EE60 * 0.006f;
    if (1.0f < *(float *)(p + 8)) {
        *(float *)(p + 8) = *(float *)(p + 8) - 1.0f;
    }
    m[8] = (int)(*(float *)(p + 8) * 255.0f);
}
