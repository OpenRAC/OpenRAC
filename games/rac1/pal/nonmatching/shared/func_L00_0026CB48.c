/* NON_MATCHING func_L00_0026CB48 -- src/overlays/shared/partupd_0026A130.c
 * Best so far: BYTES 8/552 (98.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_0026CB48 (PartType15Update): fades a particle, moves it with gravity, and on odd frames (D_L00_0015F6
 *   Two differences. (1) Retail reads the global as `lw -0x7650($gp)` with the lw in the delay slot of the first b
 *   (2) The last call's argument loads come out as f12, t2, (t3 in the slot); retail loads t3 first, f12, then t2 
 *   Unblock: the lead could declare D_L00_0015F6B0 as `short` at its gp-relative use (a separate name at exactly t
 *   z05 (fz2): type 0x15 particle update. Fixed the gp read (D_L00_0015F680 was the wrong address; D_L00_0015F6B0 
 */
#include "common.h"

typedef unsigned int u128_t __attribute__((mode(TI), aligned(16)));
typedef union { u128_t q; float f[4]; } V26;

extern int func_001F9938(void *);
extern void func_L00_002688A8(void *);
extern float func_001FA888(int);
extern int func_L00_00237B70(int, int, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_002140F8(float, float);
extern float func_001F9CB8(void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9C30(void *, void *, float);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern float D_0015EE70 MACRO_ADDR;
extern int D_L00_0015F6B0 MACRO_ADDR;
extern void *func_L00_0026CA10(void *pos, void *vel, int a2, int a3, int cnt, int s, int c, int col, float f);

/* Per-frame update of a type 0x15 particle: fade, move with gravity, occasionally spawn a spark.
   Adapted from Lombyte (MIT) for PAL: src/overlays/shared/rendering_00269290.c, FUN_L00_0026bca8. */
void func_L00_0026CB48(unsigned char *m) {
    V26 u, t, w;
    unsigned char *o = m + 0x20;
    unsigned char *p;
    float a, b;
    if (func_001F9938(m + 0xA)) {
        func_L00_002688A8(m);
        return;
    }
    p = m + 0x10;
    a = func_001FA888(*(short *)(m + 0xA));
    b = func_001FA888(*(short *)(o + 0x18));
    a *= *(float *)(o + 0x1C);
    a /= b;
    *(float *)(m + 0xC) = a;
    a = func_001FA888(*(short *)(m + 0xA));
    *(int *)(m + 4) = func_L00_00237B70(*(int *)(o + 0x14), *(int *)(o + 0x10), a / func_001FA888(*(short *)(o + 0x18)));
    func_001F9BD8(p, p, o);
    *(float *)(o + 8) -= D_0015EE70 * 14.6f;
    qcopy(&u, p);
    if (*(short *)(o + 0x1A) && (D_L00_0015F6B0 & 1)) {
        w.q = 0;
        w.f[0] = func_002140F8(-1.0f, 1.0f);
        w.f[1] = func_002140F8(-1.0f, 1.0f);
        w.f[2] = func_002140F8(-1.0f, 1.0f);
        t.q = w.q;
        b = func_001F9CB8(o);
        func_L00_001FF4B0(&t, &t, b * func_002140F8(0.15f, 0.25f));
        func_001F9BD8(&t, &t, o);
        func_001F9C30(&t, &t, func_002140F8(0.75f, 0.95f));
        {
            int n = func_001F9850(func_L00_00258BC8(0xF, 0x1E));
            unsigned char c3 = m[3];
            float f = *(float *)(m + 0xC);
            func_L00_0026CA10(p, &t, *(int *)(m + 4), *(int *)(o + 0x14), n, 0, m[2], c3, f);
        }
    } else {
        func_001F9C30(o, o, 0.96f);
    }
}
