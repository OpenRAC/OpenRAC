/* NON_MATCHING func_L00_002B30C8 -- src/overlays/shared/vendor_002AB910.c
 * Best so far: SIZE ours 804 / retail 800, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Two spark bursts around a moby (loop 1: 24*detail, func_L00_00272158; loop 2: 6*detail, func_L00_0026DA50), th
 *   Best p1.c/p4.c: 804 vs 800 bytes. Only difference: ours hoists the loop-invariant lui/addiu of the base out of
 */
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_L00_00258C80(float, float);
extern void func_002156E0(void *dst, void *vec, void *axis, float angle);
extern float func_002140F8(float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern int func_L00_00258BC8(int, int);
extern int func_002140B0(int);
extern void func_001F9C30(void *, void *, float);
extern int func_001F9850(int);
extern unsigned char *func_L00_00272158(void *pos, float *vec, int s, int a, int col, int n, float x, float y, float z, float w, float pw);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern char *func_L00_0026DA50(void *pos, void *dir, int c, int d, int n, int k, float f);
extern char D_0013E633[];

// Spawns two bursts of rotating sparks around a moby, then picks its next state.
void func_L00_002B30C8(char *moby) {
    float d[4];
    float v[4];
    float w[4];
    float u[4];
    int i, j, r, e;
    char *base2;
    *(u128 *)v = 0;
    v[0] = 0.3f;
    v[1] = -1.2f;
    v[3] = 1.0f;
    func_001F9EC0(v, v, moby + 0xC0);
    func_001F9BD8(w, moby + 0x10, v);
    for (i = 0; i < func_001FA898_r(24.0f); i++) {
        char *base = (char *)((int)D_0013E633 + 0x147D);
        func_002156E0(d, base, base - 0x20, (float)i * 0.2617994f + func_L00_00258C80(0.0f, 0.1308997f));
        func_L00_001FF4B0(d, d, func_002140F8(0.05f, 0.1f));
        r = func_L00_00258BC8(0, 3);
        if (func_002140B0(2) != 0) r = -r;
        if (*(int *)(base - 0x360) != 0) {
            func_001F9C30(u, base - 0x560, 1.7f);
            func_001F9BD8(d, d, u);
        }
        e = func_L00_00258BC8(func_001F9850(30), func_001F9850(90));
        func_L00_00272158(w, d, e, 30, 0xFFFFFF, r, 59000.0f, 3000.0f, 0.85f, -0.001f, 0.0f);
    }
    base2 = D_0013E633 + 0x147D;
    for (j = 0; j < func_001FA898_r(6.0f); j++) {
        func_002156E0(d, base2, base2 - 0x20, (float)j * 1.0471976f + func_002140F8(0.0f, 0.7853982f));
        func_L00_001FF4B0(d, d, func_002140F8(0.05f, 0.1f));
        e = func_L00_00258BC8(func_001F9850(20), func_001F9850(60));
        func_L00_0026DA50(w, d, 0x4F007FFF, 0x1FFFFFFF, e, 1, 10000.0f);
    }
    moby[0xBC] = func_001F9850(5);
}
