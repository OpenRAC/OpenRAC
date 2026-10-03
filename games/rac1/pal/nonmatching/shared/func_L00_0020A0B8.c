/* NON_MATCHING func_L00_0020A0B8 -- src/overlays/shared/help_00203E98.c
 * Best so far: SIZE ours 612 / retail 616, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns n sparks (mode 0 random near hero, 1/2 via func_L00_00250800 with alternating ids), applies scaled rand
 *   Left: retail keeps the constant 0x34 in $s5 (loop-invariant hoisted, extra saved regs s5/s6) and holds p=&sp[0
 */
typedef int u128 __attribute__((mode(TI)));
extern float func_002140F8(float, float);
extern void func_L00_00250800(void *, int, void *);
extern void func_001F9C30(void *, void *, float);
extern char *func_L00_0026FF20(char *vec, float *pos, float sc, float vy);
extern float D_0015EE6C MACRO_ADDR;

typedef struct { u128 v, p; } L20A0B8;

// Spawns n sparks around the hero, positioned by the given mode.
void func_L00_0020A0B8(int n, int mode) {
    L20A0B8 L;
    char *g = D_0013E633 + 0xE1D;
    float *v = (float *)&L.v;
    float *p = (float *)&L.p;
    int i;
    for (i = 0; i < n; ) {
        if (mode == 0) {
            v[0] = *(float *)(g + 0x80) + func_002140F8(-0.1f, 0.1f);
            v[1] = *(float *)(g + 0x84) + func_002140F8(-0.1f, 0.1f);
            v[2] = *(float *)(g + 0x88) + func_002140F8(-0.5f, 0.1f);
        } else if (mode == 1) {
            if (i & 1) func_L00_00250800(*(void **)(g + 0x2080), 0xE, v);
            else func_L00_00250800(*(void **)(g + 0x2080), 0, v);
        } else if (mode == 2) {
            if (i & 1) func_L00_00250800(*(void **)(g + 0x2080), 0x16, v);
            else func_L00_00250800(*(void **)(g + 0x2080), 0x17, v);
        }
        qcopy((char *)p, g + 0x100);
        if (*(int *)(g + 0x2084) == 0x34) func_001F9C30(p, p, 1.15f);
        else func_001F9C30(p, p, 0.7f);
        i++;
        p[0] = p[0] + func_002140F8(D_0015EE6C * -0.4f, D_0015EE6C * 0.4f);
        p[1] = p[1] + func_002140F8(D_0015EE6C * -0.4f, D_0015EE6C * 0.4f);
        p[2] = p[2] + func_002140F8(-D_0015EE6C, D_0015EE6C * 0.0f);
        func_L00_0026FF20((char *)v, p, func_002140F8(4200.0f, 7350.0f), -1.0f);
    }
}
