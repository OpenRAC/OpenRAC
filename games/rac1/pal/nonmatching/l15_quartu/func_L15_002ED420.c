/* NON_MATCHING func_L15_002ED420 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: BYTES 13/1608 (99.2% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Codebot slot update (moby class 1430): state machine over moby+0x20 (5 cases, jump table), copies a slot vecto
 *   Best: p5.c (= p6, p7 same 13 bytes), SIZE now 1608 = retail, 13 bytes differ, all in the L4A0 slot copy at the
 *   Unblock: the copy's temp register and the lui/lw order (regalloc: the lq destination is the free temp). Ten ru
 */
#include "common.h"

extern int D_L15_00160058_m __asm__("D_L15_00160058") MACRO_ADDR;
extern int D_L15_00160058_p __asm__("D_L15_00160058");
extern int D_0015EE84 MACRO_ADDR;
extern int D_0015EE98 MACRO_ADDR;
extern int D_L15_001BADE0[];
extern int D_L15_00184B2C;
extern unsigned char D_0014171B[] NOT_SDA;
extern unsigned char D_0013D355[];
extern unsigned char D_0013E633[];
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF240(void *, void *, void *);
extern void func_L15_002ED318(int, int);
extern float func_001F9D48(void *, void *);
extern void func_L10_002F6E10(int);
extern int func_L15_002F8D30(int);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BC0(void *);
extern char *func_L15_002ECDD0(char *pos, char *vec);
extern float func_00214358(void *, int, float);
extern void func_L00_00217718(void *, void *, int, int);
extern float func_001F9878(float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L15_002ED3A0(int);

/* Codebot slot update: copies its target from the slot table, steers toward it and runs its state machine. */
void func_L15_002ED420(char *moby) {
    char vA[16] __attribute__((aligned(16)));
    char vB[16] __attribute__((aligned(16)));
    char vC[16] __attribute__((aligned(16)));
    char *data = *(char **)(moby + 0x78);
    unsigned char *mb = (unsigned char *)moby;
    int idx = *(int *)(data + 0xC);

    if (idx != -1) {
        if (mb[0x20] != 0) {
            char *other = (char *)(D_L15_00160058_m + idx * 256);
            func_001F9BF0(vB, other + 0x10, data + 0x10);
            *(u128 *)vA = *(u128 *)vB;
            func_L00_001FF240(vB, moby + 0x10, vA);
        }
        {
            int ix = *(int *)(data + 0xC);
            char *o2 = (char *)(D_L15_00160058_p + ix * 256);
            *(u128 *)(data + 0x10) = *(u128 *)(o2 + 0x10);
        }
    }

    switch (mb[0x20]) {
    case 0: {
        unsigned short v = *(unsigned short *)(moby + 0xB2);
        if (D_L15_001BBB40.collected[(short)v] != 0) {
            moby[0x20] = 4;
            break;
        }
        if (((*(int *)(D_0014171B + 0xAB75 + (((short)v >> 5) * 4 + (D_0015EE84 << 8)))) >> (v & 0x1F)) & 1) {
            moby[0x20] = 4;
            break;
        }
        D_L15_00184B2C = 0;
        moby[0x20] = 1;
        {
            int *p = (int *)(data + 0x20);
            int neg = -1;
            int i = 23;
            for (; i >= 0; i--) {
                if (*p != neg) func_L15_002ED318(*p, 0);
                p++;
            }
        }
        break;
    }
    case 1:
        if (D_0013D355[0x13D] != 0) {
            unsigned char *q = D_0013E633 + 0xE9D;
            if (func_001F9D48(q, moby + 0x10) < 3.0f) {
                if (q[0x2024] == 0) {
                    func_L10_002F6E10(*(int *)data);
                    moby[0x20] = 2;
                }
            }
        }
        break;
    case 2: {
        float a;
        if (func_L15_002F8D30(*(int *)data) == 0) break;
        a = func_001F9F90(func_001FA748(*(float *)(moby + 0x48), -1.5707964f));
        *(float *)vA = a + a;
        a = func_001F9FA8(func_001FA748(*(float *)(moby + 0x48), -1.5707964f));
        *(float *)(vA + 4) = a + a;
        *(int *)(vA + 8) = 0;
        func_L00_001FF240(vC, vA, moby + 0x10);
        func_001F9BC0(vB);
        *(float *)(vB + 8) = func_001FA748(*(float *)(moby + 0x48), 1.5707964f);
        *(float *)(vB + 4) = 1.5707964f;
        *(char **)(data + 4) = func_L15_002ECDD0(vA, vB);
        *(int *)(data + 8) = 0;
        *(float *)vA = func_001F9F90(func_001FA748(*(float *)(moby + 0x48), -1.5707964f)) * 3.0f;
        *(float *)(vA + 4) = func_001F9FA8(func_001FA748(*(float *)(moby + 0x48), -1.5707964f)) * 3.0f;
        *(int *)(vA + 8) = 0;
        func_L00_001FF240(vC, vA, moby + 0x10);
        *(float *)(vA + 8) = func_00214358(vA, 0, 0.5f);
        func_001F9BC0(vB);
        *(float *)(vB + 8) = func_001FA748(*(float *)(moby + 0x48), 1.5707964f);
        func_L00_00217718(vA, vB, 0x72, 0);
        {
            int d = *(int *)(data + 0x20);
            if (d != -1) func_L15_002ED318(d, 1);
        }
        moby[0x20] = 3;
        break;
    }
    case 3: {
        char *d4;
        float f20v;
        if (*(char **)(data + 4) == 0) break;
        f20v = 360.0f / func_001F9878(360.0f);
        *(float *)(data + 8) = func_001FA748(*(float *)(data + 8), f20v * 0.017453292f);
        f20v = func_001F9F90(*(float *)(data + 8)) + 1.0f;
        *(float *)vA = func_001F9F90(func_001FA748(*(float *)(moby + 0x48), -1.5707964f)) * f20v;
        *(float *)(vA + 4) = func_001F9FA8(func_001FA748(*(float *)(moby + 0x48), -1.5707964f)) * f20v;
        *(int *)(vA + 8) = 0;
        d4 = *(char **)(data + 4);
        func_001F9BD8(vB, moby + 0x10, vA);
        *(u128 *)(d4 + 0x10) = *(u128 *)vB;
        if (f20v < 0.5f) {
            {
                unsigned short v1 = *(unsigned short *)(moby + 0xB2);
                *(int *)(D_0014171B + 0xAB75 + (((short)v1 >> 5) * 4 + (D_0015EE84 << 8))) |= 1 << (v1 & 0x1F);
            }
            {
                unsigned short v2 = *(unsigned short *)(moby + 0xB2);
                D_L15_001BADE0[(short)v2 >> 5] |= 1 << (v2 & 0x1F);
            }
            moby[0x20] = 4;
        }
        break;
    }
    case 4: {
        char *d4;
        D_L15_00184B2C = 1;
        d4 = *(char **)(data + 4);
        if (d4 == 0) {
            float a = func_001FA748(*(float *)(moby + 0x48), -1.5707964f);
            *(float *)vA = func_001F9F90(a) * 0.5f;
            *(float *)(vA + 4) = func_001F9FA8(func_001FA748(*(float *)(moby + 0x48), -1.5707964f)) * 0.5f;
            *(int *)(vA + 8) = 0;
            func_L00_001FF240(vC, vA, moby + 0x10);
            func_001F9BC0(vB);
            *(float *)(vB + 8) = func_001FA748(*(float *)(moby + 0x48), 1.5707964f);
            *(float *)(vB + 4) = 1.5707964f;
            *(u128 *)vC = *(u128 *)(moby + 0x10);
            *(char **)(data + 4) = func_L15_002ECDD0(vC, vB);
        } else {
            float a = func_001FA748(*(float *)(moby + 0x48), -1.5707964f);
            *(float *)vA = func_001F9F90(a) * 0.5f;
            *(float *)(vA + 4) = func_001F9FA8(func_001FA748(*(float *)(moby + 0x48), -1.5707964f)) * 0.5f;
            *(int *)(vA + 8) = 0;
            func_L00_001FF240(vC, vA, moby + 0x10);
            func_001F9BC0(vB);
            *(float *)(vB + 8) = func_001FA748(*(float *)(moby + 0x48), 1.5707964f);
            *(float *)(vB + 4) = 1.5707964f;
            *(u128 *)(*(char **)(data + 4) + 0x10) = *(u128 *)vA;
            *(u128 *)(*(char **)(data + 4) + 0x40) = *(u128 *)vB;
        }
        {
            unsigned char *b = D_0013D355 + 0x13B;
            if (b[0x6D] == 0) {
                if (func_L15_002ED3A0(*(int *)(data + 0x20)) >= 10) {
                    b[0x6D] = 1;
                    D_0015EE98 += 0x3A98;
                }
            }
        }
        break;
    }
    default:
        break;
    }
}
