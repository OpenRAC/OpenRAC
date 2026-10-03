/* NON_MATCHING func_L00_00205278 -- src/overlays/shared/help_00203E98.c
 * Best so far: SIZE ours 788 / retail 796, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   HeroInit: copies 7 words (stride 0x50) out of the hero block twice around a memset of the block, binds the fir
 *   Best candidate p7.c (788 vs 796 bytes): same instruction mix; remaining differences are register assignment (r
 *   Found: per-block `char *x = D_0013E633 + 0xE1D` locals give the unfolded lui/addiu pairs; `(30 - i) * 0xB0` wi
 */
#include "common.h"
extern unsigned char D_0013E633[] NOT_SDA;
extern unsigned char D_0014171B[] NOT_SDA;
extern unsigned char D_0013E15A[] NOT_SDA;
extern int D_L00_0016009C MACRO_ADDR;
extern int D_L00_00160098 MACRO_ADDR;
extern int D_L00_00160600 MACRO_ADDR;
extern char D_L00_0017A780[];
extern int D_0015EEA0 MACRO_ADDR;
extern void func_001F99B0(void *, int, int);
extern float func_00214358(void *, int, float);
extern void func_L00_00212E70(void);
extern void func_L00_00208E98(void);
extern void func_001F9BC0(float *);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);

/* resets the hero state block and its moby slot, then seeds the heading tables */
void func_L00_00205278(void) {
    int saved[7];
    char *g;
    char *a = (char *)D_0013E633 + 0xE1D;
    char *gm;
    char *w;
    int i;
    int j;
    int k;
    int l;
    char *p;
    char *e;
    int r;
    int end;
    char *s;
    char *t;
    for (i = 6; i >= 0; i--) saved[6 - i] = *(int *)(a + 0x10B8 + (6 - i) * 0x50);
    g = (char *)D_0013E633 + 0xE1D;
    func_001F99B0(g, 0, 0x2310);
    for (j = 6; j >= 0; j--) saved[6 - j] = *(int *)(g + 0x10B8 + (6 - j) * 0x50);
    end = D_L00_0016009C;
    for (p = (char *)D_L00_00160098; p < (char *)end; p += 0x100) {
        if (*(short *)(p + 0xA6) == 0) {
            float f;
            gm = (char *)D_0013E633 + 0xE1D;
            *(int *)(gm + 0x2080) = (int)p;
            f = func_00214358(p + 0x10, 0, 0.5f);
            if (0.0f < f) *(float *)(*(int *)(gm + 0x2080) + 0x18) = f;
            *(unsigned short *)(*(int *)(gm + 0x2080) + 0x34) |= 2;
            *(int *)(gm + 0xA88) = *(int *)(gm + 0x2080);
            *(float *)(gm + 0x98) = *(float *)(*(int *)(gm + 0x2080) + 0x48);
            qcopy(gm + 0x80, (char *)*(int *)(gm + 0x2080) + 0x10);
            func_L00_00212E70();
            func_L00_00208E98();
            if (D_L00_00160600 == 0) {
                qcopy(D_0013E15A + 0x36, gm + 0x80);
                qcopy(D_0013E15A + 0x46, D_0013E633 + 0xEAD);
            }
            break;
        }
    }
    s = D_L00_0017A780;
    t = s + 1;
    if (*(int *)(D_0014171B + 0x45) == 0) *(int *)(D_0014171B + 0x45) = 10;
    for (k = 30; k >= 0; k--) {
        *(t + (30 - k) * 0xB0) = 0;
        func_001F9BC0((float *)(s + (30 - k) * 0xB0 + 0x60));
        func_001F9BC0((float *)(s + (30 - k) * 0xB0 + 0x40));
        func_001F9BC0((float *)(s + (30 - k) * 0xB0 + 0x50));
        func_001F9BC0((float *)(s + (30 - k) * 0xB0 + 0x90));
        func_001F9BC0((float *)(s + (30 - k) * 0xB0 + 0x70));
        func_001F9BC0((float *)(s + (30 - k) * 0xB0 + 0x80));
    }
    w = (char *)D_0013E633 + 0xE1D;
    for (l = 7; l >= 0; l--) *(int *)(w + 0x2218 + l * 4) = -1;
    g = (char *)D_0013E633 + 0xE1D;
    *(int *)(g + 0x22A8) = D_0015EEA0;
    *(float *)(g + 0x2288) = 2.125f;
    *(float *)(g + 0x228C) = 1.25f;
    *(int *)(g + 0x22AC) = 4;
    *(short *)(g + 0x22B0) = *(unsigned short *)(g + 0x22A8);
    r = func_001F9850(0xB4);
    *(int *)(g + 0x1010) = func_L00_00258BC8(r, func_001F9850(0x12C));
    *(float *)(g + 0xD20) = 0.97f;
    *(float *)(g + 0x1014) = 0.0070f;
    *(float *)(g + 0x1018) = 0.3f;
}
