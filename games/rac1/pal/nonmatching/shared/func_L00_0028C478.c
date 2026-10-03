/* NON_MATCHING func_L00_0028C478 -- src/overlays/shared/shrubproc_0028A198.c
 * Best so far: BYTES 6/660 (99.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_0028C478: rebuilds the sky-shell rotation matrices for each layer (switch on layer 1..5 picks a spin 
 *   p3.c / p4.c are identical to retail except one swap at the end: retail emits `addiu $s0,$s0,0x30` before the `
 *   Would need a qcopy-free 16-byte copy form or a different scheduling of the inline-asm volatile barrier.
 *   Second pass (p5-p9, Lombyte port + index local): 6 bytes differ, only the swap of `addiu $s0,$s0,0x30` and `lu
 *   Would unblock: a copy form that evaluates the destination before the source without folding the offset; not re
 */
#include "common.h"
extern char D_L00_001BDB70[] NOT_SDA;
extern char *D_L00_001605DC MACRO_ADDR;
extern short D_L00_00160580;
extern float D_L00_001605E0[];
extern void func_001FA190(void *);
extern void func_001F9BC0(void *);
extern void func_L00_001FFA40(void *, void *);
extern void func_001F9C48(void *, void *, float);
extern void func_0022C9A8(int);

/* Rebuilds the sky shell rotation matrices for each layer and draws them. Adapted from Lombyte (MIT) for PAL: overlays/shared/unclassified_00288ec0.c, FUN_L00_0028b1a0. */
void func_L00_0028C478(void) {
    float v[4] __attribute__((aligned(16)));
    int i = 0;
    float *p;
    unsigned char *M;
    float k;
    *(short *)(D_L00_001605DC + 4) = 0;
    func_001FA190(D_L00_001BDB70);
    func_001F9BC0(v);
    for (; i < *(short *)(D_L00_001605DC + 6); i++) {
        int o = i * 4;
        p = (float *)(*(char **)(D_L00_001605DC + o + 0x20) + 8);
        k = 1.0f;
        switch (i) {
        case 1:
            v[1] = 0.02f;
            k = 3.0f;
            *p += *(float *)&D_L00_00160580 * 0.55f;
            break;
        case 2:
            v[1] = -0.02f;
            k = 2.5f;
            *p += *(float *)&D_L00_00160580 * 0.6f;
            break;
        case 3:
            v[1] = 0.01f;
            k = 2.0f;
            *p += *(float *)&D_L00_00160580 * 0.7f;
            break;
        case 4:
            v[1] = -0.01f;
            k = 1.5f;
            *p += *(float *)&D_L00_00160580 * 0.75f;
            break;
        case 5:
            k = 1.25f;
            *p += *(float *)&D_L00_00160580 * 0.8f;
            break;
        default:
            func_001FA190(D_L00_001BDB70);
            break;
        }
        if (i > 0) {
            M = D_L00_001BDB70;
            v[2] = (float)((int)(*p + i * 2000.0f) % 10000) * 0.00062831853f - 3.1415927f;
            func_L00_001FFA40(M, v);
            func_001F9C48(M, M, k);
            func_001F9C48(M + 0x10, M + 0x10, k);
            func_001F9C48(M + 0x20, M + 0x20, k);
            {
                char *dst = M + 0x30;
                float *src = D_L00_001605E0;
                qcopy(dst, src);
            }
        }
        func_0022C9A8(i);
    }
}
