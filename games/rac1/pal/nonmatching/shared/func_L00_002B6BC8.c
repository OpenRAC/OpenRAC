/* NON_MATCHING func_L00_002B6BC8 -- src/overlays/shared/vendor_002B33E8.c
 * Best so far: BYTES 8/580 (98.6% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns the camera-collision-line moby (0xAC bytes): fills its data block, starts helper mobys/effects, and sna
 *   Only difference: delay slot of the jal func_L00_00222B80(0x1D, 1). Retail orders li a0 / li a1 / sw v0,0x14(v0
 *   Key points that got it there: the second read of D_0013E633+0xE1D (offset 0x2080) must be a separate pointer l
 */
#include "common.h"
typedef struct { float f[4]; } V __attribute__((aligned(16)));
extern char D_0013E633[];
extern int D_L00_0017E608;
extern int D_L00_0015F6E8 MACRO_ADDR;
extern short D_L00_0016156C;
extern float D_0015EE6C MACRO_ADDR;
extern char D_L00_00173F60[];
extern char *func_0020D348(int);
extern float func_001F9CB8(void *);
extern int func_001F9850(int);
extern void func_L00_002B69C0(void *);
extern void func_L00_00251E30(void *);
extern char *func_L00_002ECDE0(char *);
extern void func_L00_00222B80();
extern void func_001F99B0(void *, int, int);
extern void func_0020D960(char *, int, void *);
extern char *func_L00_002BC668(char *);
extern void func_L00_0025E210(void *);
extern int func_0022ED80(int, int, int);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern int func_00216028(int, int);

/* Spawns the camera collision-line moby, wires up its data block and helper mobys, and snaps it to the ground when a line test hits. Adapted from Lombyte (MIT) for PAL: overlays/shared/unclassified_002b2100.c, FUN_L00_002b58d8. */
int func_L00_002B6BC8(void *a, void *pos, float fa, float fb) {
    unsigned char *m = (unsigned char *)func_0020D348(0xAC);
    if (m) {
        char *v = *(char **)(m + 0x78);
        char *G = D_0013E633 + 0xE1D;
        V t;
        char *g2;
        *(unsigned char **)(G + 0x1FE0) = m;
        D_L00_0017E608 = 1;
        m[0x30] = 0xFF;
        *(short *)(m + 0x32) = 0xFF;
        m[0x31] = 1;
        m[0x20] = 1;
        *(float *)(m + 0x48) = fa;
        *(float *)(m + 0x44) = fb;
        *(char **)(v + 0x24) = G + 0x1F60;
        *(char **)(v + 0x28) = G + 0x1FA0;
        *(void **)(v + 0x0) = a;
        qcopy(m + 0x10, pos);
        *(int *)(v + 0x8) = 0;
        *(int *)(v + 0xC) = 0;
        if (*(char **)(G + 0x2FC) && func_L00_0025D390(*(char **)(G + 0x2FC)))
            *(float *)(v + 0x4) = func_001F9CB8(G + 0x100);
        else
            *(int *)(v + 0x4) = 0;
        *(int *)(v + 0x10) = func_001F9850(0xBB8);
        *(int *)(v + 0x18) = 0;
        *(int *)(v + 0x1C) = 0;
        *(int *)(v + 0x14) = 0;
        *(float *)(v + 0x44) = 0.4f;
        func_L00_002B69C0(m);
        func_L00_00251E30(m);
        *(int *)(v + 0x14) = (int)func_L00_002ECDE0((char *)m);
        func_L00_00222B80(0x1D, 1);
        func_001F99B0(*(void **)(v + 0x24), 0, 0x40);
        func_001F99B0(*(void **)(v + 0x28), 0, 0x40);
        func_0020D960(m, 0, *(void **)(v + 0x24));
        func_0020D960(m, 1, *(void **)(v + 0x28));
        {
            char *r = func_L00_002BC668(m);
            float k = *(float *)&D_L00_0016156C * D_0015EE6C;
            *(char **)(v + 0x20) = r;
            *(float *)(v + 0x48) = k;
        }
        func_L00_0025E210(m);
        *(int *)(v + 0x2C) = func_0022ED80(0, 4, (int)m);
        g2 = D_0013E633 + 0xE1D;
        t = *(V *)((char *)a + 0x10);
        t.f[2] = ((V *)pos)->f[2];
        if (func_L00_001EFFF0(&t, pos, 0, *(int *)(g2 + 0x2080), 0)) {
            m[0xBC] = 1;
            qcopy(m + 0x10, D_L00_00173F60);
        }
        D_L00_0015F6E8 = 1;
        func_00216028(6, 0);
    }
    return (int)m;
}
