/* NON_MATCHING func_L00_001EC220 -- src/overlays/shared/camera_001EB508.c
 * Best so far: SIZE ours 592 / retail 596, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_001EC220: camera transition commit. Switches on D_L00_00166FF0[0]==1 and the mode byte at +3, copies/
 *   p2.c matches retail instruction for instruction except the prologue: ours saves one extra register ($s6 keeps 
 *   w06 round: best p6.c (592/596, ONE instruction differs: retail has `daddu $a0,$a1,$zero` (copy of the loaded g
 */
#include "common.h"
extern char D_0013E633[];
extern char D_L00_00166FF0[];
extern short D_L00_00166FF0_s[] __asm__("D_L00_00166FF0");
extern void func_00215328(void *, void *);
extern void func_L00_001EC090(void);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001EC8D8(float *out, void *p0, void *p1, void *dir0, void *dir1, void *axis);
extern void func_001ECB98(void);
extern void func_001ECC10(void);
extern int func_001F9850(int);
extern float func_001FA888(int);

/* commits the camera transition mode chosen this frame and updates its blend state */
void func_L00_001EC220(char *a) {
    char *g = D_L00_00166FF0;
    int t;
    unsigned char m;
    if (*(short *)D_L00_00166FF0 == 1) {
        m = g[3];
        if (m == 0) {
            qcopy(g + 0x50, a + 0x30);
            func_00215328(g + 0x60, a);
        } else if (m == 2) {
            qcopy(g + 0xC0, a + 0x30);
            func_00215328(g + 0xD0, a);
            func_L00_001EC090();
        } else {
            char *base = (char *)D_0013E633 + 0xE1D;
            float one = 1.0f;
            float v0[4];
            float v1[4];
            float v2[4];
            char *q = g + 0xB0;
            func_L00_001FF4B0(v0, *(char **)(base + 0x2080) + 0xC0, one);
            func_L00_001FF4B0(v1, *(char **)(base + 0x2080) + 0xD0, one);
            func_L00_001FF4B0(v2, *(char **)(base + 0x2080) + 0xE0, one);
            func_001EC8D8((float *)(g + 0x70), a + 0x30, *(char **)(g - 0xF0) + 0x30, v0, v1, v2);
            func_00215328(q, a);
            qcopy(g + 0xD0, q);
        }
    } else {
        m = g[3];
        if (m == 2) {
            func_001ECB98();
            func_L00_001EC090();
        } else if (m == 1) {
            func_001ECB98();
            qcopy(g + 0xB0, g + 0xD0);
        } else if (m == 0) {
            func_001ECC10();
        }
    }
    t = (unsigned char)g[3];
    D_L00_00166FF0_s[0] = 3;
    g[2] = t;
    if (t == 0) {
        char *h = g + 0x10;
        *(int *)(h + 0xC) = 0;
        *(float *)(h + 0x10) = *(float *)(h + 0x14);
        qcopy(g + 0x40, g + 0x50);
        *(int *)(g + 0x10) = 0;
        *(float *)(h + 4) = *(float *)(h + 8);
        qcopy(g + 0x30, g + 0x60);
    } else {
        char *r = g + 0x70;
        int k = *(int *)(r + 0x14) + 1;
        *(int *)(r + 0x14) = k;
        k = func_001F9850(k);
        *(int *)(r + 0xC) = k;
        *(float *)(r + 0x10) = 1.0f / func_001FA888(k);
    }
}
