/* NON_MATCHING func_L10_002E4E50 -- src/overlays/l10_orxon/vendor_002E30F8.c
 * Best so far: SIZE ours 840 / retail 856, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Per-frame update of the moby in arg0: copies arg0+0x10 into a vector, scales two trig-derived vectors (a, b) f
 *   Stopped at a wall (QUEUE.md): retail reloads D_0015EE6C with a fresh `lui $1, %hi` and `lwc1` before each use 
 */
extern int D_L10_00161EF4 SDATA(D_L10_00161EF4);
extern int D_L10_00161EF8 SDATA(D_L10_00161EF8);
extern float D_L10_00161EFC SDATA(D_L10_00161EFC);
extern float D_L10_00161F00 SDATA(D_L10_00161F00);
extern float D_L10_00161F04 SDATA(D_L10_00161F04);
extern float D_L10_00161F08 SDATA(D_L10_00161F08);
extern float D_L10_00161F0C SDATA(D_L10_00161F0C);
extern float D_L10_00161F10 SDATA(D_L10_00161F10);
extern float D_L10_00161F14 SDATA(D_L10_00161F14);
extern float D_L10_00161F18 SDATA(D_L10_00161F18);
extern float D_L10_00161F1C SDATA(D_L10_00161F1C);
extern float D_L10_00161F20 SDATA(D_L10_00161F20);
extern int D_L10_00161F24 SDATA(D_L10_00161F24);
extern int D_L10_00161F28 SDATA(D_L10_00161F28);
extern int D_L10_00161F2C SDATA(D_L10_00161F2C);
extern int D_L10_00161F30 SDATA(D_L10_00161F30);
extern int D_L10_00161F34 SDATA(D_L10_00161F34);
extern int D_L10_00161F38 SDATA(D_L10_00161F38);
extern int D_L10_00161F3C SDATA(D_L10_00161F3C);
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern void func_L00_00260958(float *v, float s);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9850(int);
extern int func_001FA8A8(int, int, float);
extern float func_001F9878(float);
extern s32 func_001FA898(f32);
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);

// Per-frame update of the moby in arg0: builds two vectors from its position and the trig helpers, then calls the level's helpers for each of its entries.
void func_L10_002E4E50(void *p) {
    char *arg0 = p;
    float v[4];
    float a[4];
    float b[4];
    float o[4];
    char *d = *(char **)(arg0 + 0x78);
    float f20;
    float f21;
    float f22;
    float r;
    int n = D_L10_00161EF4;
    int i;
    int j;
    int k;
    int x16, x17, x18, x19, r7;
    for (i = 0; i < n; i++) {
        qcopy(v, arg0 + 0x10);
        func_L00_00260958(v, D_L10_00161EFC);
        v[2] = v[2] + 0.55f;
        f22 = func_00214158();
        f20 = func_002140F8(D_L10_00161F08, D_L10_00161F0C);
        f21 = func_002140F8(D_L10_00161F10, D_L10_00161F14);
        if (((unsigned char)arg0[0x20]) == 0xB) {
            f20 = f20 * 0.55f;
            f21 = f21 * 0.55f;
            v[2] = v[2] - 0.2f;
        }
        r = func_001F9F90(f22);
        a[0] = r * (f20 * D_0015EE6C);
        r = func_001F9FA8(f22);
        a[1] = r * (f20 * D_0015EE6C);
        a[2] = (f21 * D_0015EE6C) + 0.0f;
        func_001F9C30(o, d + 0x70, 0.5f);
        func_001F9BD8(a, a, o);
        qcopy(b, a);
        k = func_001F9850(D_L10_00161F38);
        b[2] = b[2] - (D_L10_00161F18 * D_0015EE70) * (float)k;
        f22 = 0.5f;
        f21 = 0.0f;
        f20 = 1.0f;
        for (j = 0; j < D_L10_00161EF8; j++) {
            r = func_002140F8(f22, 1.5f);
            a[3] = r * D_L10_00161F1C;
            b[3] = r * D_L10_00161F20;
            func_L00_00260958(v, D_L10_00161F00);
            func_L00_00260958(b, D_L10_00161F04 * D_0015EE6C);
            r = func_002140F8(f21, f20);
            x19 = func_001FA8A8(D_L10_00161F24, D_L10_00161F2C, r);
            r = func_002140F8(f21, f20);
            func_001FA8A8(D_L10_00161F28, D_L10_00161F30, r);
            x18 = func_001F9850(D_L10_00161F34);
            x17 = func_001F9850(D_L10_00161F38);
            x16 = x17;
            r = func_002140F8((float)D_L10_00161F3C * 0.5f, (float)D_L10_00161F3C * 2.5f);
            r = func_001F9878(r);
            r7 = func_001FA898(r);
            func_00219780(v, a, b, x19, x18, x17, x16, r7, -1);
        }
    }
}
