/* NON_MATCHING func_L00_00207EC0 -- src/overlays/shared/help_00203E98.c
 * Best so far: BYTES 20/316 (93.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Camera-shake helper (port of Lombyte FUN_L00_002078a8): for mode 0x7F calls func_L00_00263B78 with a scaled D_
 *   calls it and then rotates a small vector (0,0,0.21) and adds it to the object's position. p0 is 20 of 316 byte
 *   argument scheduling, retail loads a0 (lw 0x2080) before the f12 constant and puts mul.s in the jal delay slot;
 *   The second call, identical in form, schedules like retail. Locals for a/y, operand order, goto form (p1-p5) al
 *   Unblock: some wording that changes the first block's scheduling; nothing found.
 */
typedef unsigned int u128_p __attribute__((mode(TI), aligned(16)));
typedef union { u128_p q; float f[4]; } Vec4_p;
typedef struct { char p0[0x6A8]; float f6A8; float f6AC; char p1[0x86C - 0x6B0]; char *o86C; char p2[0x8BC - 0x870]; short h8BC; char p3[0x2080 - 0x8BE]; char *w2080; int w2084; } S_p;
extern unsigned char D_0013E633[] NOT_SDA;
extern float D_0015EE6C MACRO_ADDR;
extern char D_L00_0017A7C0[] NOT_SDA;
extern void func_L00_00263B78(float, float, char *, float *, float *);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001FA218(void *, void *);
extern void func_001F9EE8(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);

/* Applies the camera-shake offsets (rotation and a small vector) for modes 0x7F and 0x6B. Adapted from Lombyte (MIT) for PAL: overlays/shared/ui_help_00203b18.c, FUN_L00_002078a8. */
void func_L00_00207EC0(void) {
    S_p *P = (S_p *)((char *)D_0013E633 + 0xE1D);
    Vec4_p v;
    float m[16] __attribute__((aligned(16)));
    if (P->w2084 == 0x7F) {
        float *p = &P->f6A8;
        float *q = &P->f6AC;
        char *a = P->w2080;
        func_L00_00263B78(0.3f, D_0015EE6C * 6.981317f, a, p, q);
    }
    if (P->w2084 == 0x6B && P->h8BC == 0) {
        func_L00_00263B78(1.1f, D_0015EE6C * 5.2359877f, P->w2080, &P->f6A8, &P->f6AC);
        if (P->o86C) {
            char *s = P->w2080;
            qcopy(P->o86C + 0x10, s + 0x10);
            v.q = 0;
            v.f[2] = 0.21f;
            func_001F9EC0(&v, &v, s + 0xC0);
            func_001FA218(m, D_L00_0017A7C0);
            func_001F9EE8(&v, &v, m);
            func_001F9BD8(P->o86C + 0x10, P->o86C + 0x10, &v);
        }
    }
}
