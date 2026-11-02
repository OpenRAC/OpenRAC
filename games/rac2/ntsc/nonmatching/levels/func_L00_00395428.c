/* Reconstructed on 2026-10-23, default -O2 -G8 -ffunction-sections.
 * US and US2: 1360/1428 bytes, 0/27 placements each; absent from PAL.
 * Earlier variants produced 1376 or 1392 bytes. Exact float constants, a shared
 * five-vector table identity, pointer-loop initialization,
 * world-field layouts and explicit SDA scalar declarations were tested. Retail
 * contains ordinary madd address calculations, not unsupported packed copies.
 * Remaining differences include address accumulation, register allocation and
 * scheduling. Reopen with an independently supported address/data model.
 * The older comment below is preserved as historical provenance, not a conclusion.
 */
/* MMI (SPECIAL2) instruction: no C spelling. 1428 bytes, level
   00_aranos_tutorial. Retail at 0x00395428 carries the SPECIAL2 words
   0x72821800, 0x72821800 (+0x030C, +0x049C) - opcode 0x1C, which rabbitizer
   prints as `INVALID`; it is the MMI family (pcpyld and its relatives) the
   sixth wave recorded and that no C spelling reaches. This cc1 emits
   `lq`/`sq` for a `mode(TI)` local and `ldl`/`ldr`/`sdl`/`sdr` for a struct
   assignment, never the packed copy. ASM_FUNC candidate, as for the rest of
   the family. */

#include "include_asm.h"
typedef int Quadword __attribute__((mode(TI)));
typedef union { Quadword q; float f[4]; } Vector;
typedef struct { char pad[0x2290]; char *object; int mode; } WorldState;
extern WorldState D_00189E20;
extern float D_L00_0025C64C;
extern short D_L00_001B2778[], D_L00_001B2790[], D_L00_001B27A8[], D_L00_001B27C0[];
extern int D_L00_001AF278 __attribute__((sda));
extern int D_L00_001AF27C __attribute__((sda));
extern float D_L00_001AF274 __attribute__((sda));
extern float D_L00_001B27DC __attribute__((sda));
extern int D_L00_001B27D4, D_L00_001B27D8;
extern Vector D_L00_00239EF0[][5];
extern Vector D_L00_001B8DC0;
extern void func_00282D30(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern float func_0029F530(void *);
extern int func_00282948(int *);
extern float func_00283CE0(int);
extern void func_002A7470(void *, int, int, int);
extern void func_002A7988(void *, float, float);
extern float func_002A7878(float, float);
extern float func_002A78D8(float, float);
extern void func_002ACEC0(void *, void *, void *, float);
extern void func_L00_003959C0(void);
extern void func_L00_002E6EF8(void *, void *);

void func_L00_00395428(char *object)
{
    Vector origin, random, difference, offsets[5];
    int emitting = 0;
    int i, j, active;
    float distance;
    if (*(unsigned char *)(D_00189E20.object + 0x43) != 0x7B)
        D_L00_0025C64C = 0.1f;
    switch (*(unsigned char *)(object + 0x20)) {
    case 0:
        *(unsigned char *)(object + 0x20) = 2;
        {
            short *a = D_L00_001B2778, *b = D_L00_001B27A8, *c = D_L00_001B27C0;
            for (i = 9; i >= 0; i--) {
                *a++ = 0;
                *b++ = 2;
                *c++ = 1;
            }
        }
        D_L00_001B27D4 = D_L00_001AF278;
        D_L00_001B27D8 = D_L00_001AF27C;
        break;
    case 2:
        if (D_00189E20.mode == 31) {
            distance = func_0029F530(object);
            if (distance >= 6.0f) {
                emitting = 1;
                if (!func_00282948(&D_L00_001B27D4)) {
                    float elapsed = func_00283CE0(D_L00_001B27D4);
                    elapsed /= func_00283CE0(D_L00_001AF278);
                    D_L00_001B27DC = 1.0f - elapsed;
                } else if (distance >= 12.0f) {
                    if (!func_00282948(&D_L00_001B27D8)) {
                        float elapsed = func_00283CE0(D_L00_001B27D8);
                        elapsed /= func_00283CE0(D_L00_001AF27C);
                        D_L00_001B27DC = elapsed;
                    } else {
                        D_L00_001B27DC = 0.0f;
                    }
                } else {
                    D_L00_001B27DC = 1.0f;
                }
            }
        } else {
            D_L00_001B27D4 = D_L00_001AF278;
            D_L00_001B27D8 = D_L00_001AF27C;
            func_002A7470(object, 1, 0, 12);
        }
        break;
    }
    func_00282D30(&origin, object + 0xD0, -0.5f);
    func_00282CC0(&origin, &origin, object + 0x10);
    active = 0;
    for (i = 0; i < 10; i++) {
        if (D_L00_001B2778[i]) {
            active = 1;
            break;
        }
    }
    if (!emitting && !active)
        return;
    for (i = 0; i < 10; i++) {
        if (--D_L00_001B2778[i] < 0)
            D_L00_001B2778[i] = 0;
        if (!D_L00_001B2778[i]) {
            if (!emitting)
                continue;
            D_L00_001B2778[i] = 2;
            D_L00_001B2790[i] = 64;
            func_002A7988(&random, D_L00_001AF274, D_L00_001AF274);
            D_L00_00239EF0[i][0].q = origin.q;
            {
                float sign = 1.0f;
                Vector *input = &D_L00_00239EF0[i][0];
                Vector *output = &D_L00_00239EF0[i][1];
                for (j = 3; j >= 0; j--) {
                    func_00282CF0(&difference, input, &D_L00_001B8DC0);
                    func_002ACEC0(&random, &random, &difference,
                                 func_002A7878(0.174532949924469f, 0.7853982448577881f) * sign);
                    sign = -sign;
                    func_00282CC0(output++, input++, &random);
                }
            }
        } else {
            Vector *input = &D_L00_00239EF0[i][1];
            Vector *previous = &D_L00_00239EF0[i][0];
            Vector *out = &offsets[1];
            for (j = 3; j >= 0; j--) {
                func_00282CF0(out, input++, previous++);
                out->f[0] += func_002A78D8(0.0f, 0.1f);
                out->f[1] += func_002A78D8(0.0f, 0.1f);
                out->f[2] += func_002A78D8(0.0f, 0.1f);
                out++;
            }
            D_L00_00239EF0[i][0].q = origin.q;
            input = &D_L00_00239EF0[i][0];
            out = &D_L00_00239EF0[i][1];
            previous = &offsets[1];
            for (j = 3; j >= 0; j--)
                func_00282CC0(out++, input++, previous++);
            if (D_L00_001B2778[i] > 0)
                D_L00_001B2790[i] = 32;
            else
                D_L00_001B2790[i] = 0;
        }
    }
    func_L00_002E6EF8(func_L00_003959C0, object);
}
