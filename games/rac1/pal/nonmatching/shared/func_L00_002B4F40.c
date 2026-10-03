/* NON_MATCHING func_L00_002B4F40 -- src/overlays/shared/vendor_002B33E8.c
 * Best so far: SIZE ours 1244 / retail 1252, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002B4F40(m): particle burst around a moby; spawns trail particles via func_L00_0026DA50/DEA0, then co
 *   Best candidate p8.c (size matches, first ~0x250 bytes exact). Remaining: (1) retail loads the 1.075f constant 
 *   Unblock: a way to declare the same symbol both small (gp) and MACRO_ADDR in one file, and the source order tha
 */
#include "common.h"
extern float func_00214158(void);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_002140F8(float, float);
extern void func_001F9EC0(void *, void *, void *);
extern int func_001F9850(int);
extern char *func_L00_0026DA50(void *pos, void *dir, int c, int d, int n, int k, float f);
extern float func_001F9D10(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_002140B0(int);
extern void *func_L00_0026DEA0(void *pos, int spd, void *pos2, int col, float a, float b, float c, float d);
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE6C_g __asm__("D_0015EE6C");
extern short D_L00_0015F6B0;
extern char D_0013E633[];

/* Spawns a burst of particles around a moby and refreshes its trail position. */
void func_L00_002B4F40(char *m) {
    float a[4], b[4], c[4], e[4];
    char *data = *(char **)(m + 0x78);


    char *pos, *rot, *g;
    int col, n, i;
    float d, s;
    void *p;

    a[0] = func_001F9F90(func_00214158()) * 0.05f;
    a[1] = func_001F9FA8(func_00214158()) * 0.05f;
    a[2] = 0;
    a[2] = -func_002140F8(0.01f, 0.03f);
    func_001F9EC0(a, a, m + 0xC0);
    
    qcopy(b, m + 0x10);

    if (*(int *)(D_0013E633 + 0xE1D + 0x2084) == 0x81) {
        if (*(int *)&D_L00_0015F6B0 % 3 == 0)
            func_L00_0026DA50(b, a, 0x4F007FFF, 0x1FFFFFFF, func_001F9850(10), 1, 10000.0f);
    } else {
        func_L00_0026DA50(b, a, 0x4F007FFF, 0x1FFFFFFF, func_001F9850(10), 1, 10000.0f);
    }
    pos = m + 0x10;
    rot = m + 0xC0;
    if (*(int *)(data + 0xC) != 0) {
        d = func_001F9D10(data + 0x10, b) * 0.5f;
        func_001F9BF0(c, data + 0x10, b);
        for (i = 1; i < 2; i++) {
            func_L00_001FF4B0(e, c, d * i);
            func_001F9BD8(e, b, e);
            func_L00_0026DA50(e, a, 0x4F007FFF, 0x1FFFFFFF, func_001F9850(10), 1, 10000.0f);
        }
    }
    g = D_0013E633 + 0xE1D;
    *(int *)data = func_002140B0(func_001F9850(10));
    col = 0x808080;
    s = 1.075f;
    a[0] = 0;
    a[1] = 0;
    a[2] = -0.3f;
    a[3] = 1.0f;
    func_001F9EC0(a, a, rot);
    func_001F9BD8(b, pos, a);
    a[3] = 1.0f;
    a[2] = -0.075f;
    a[0] = 0;
    a[1] = 0;
    func_001F9EC0(a, a, rot);
    a[2] = 0.025f;
    if (*(int *)(g + 0x2084) == 8) {
        col = 0xA0A0A0;
        s = 1.0165f;
        a[2] = 0.025f - D_0015EE6C * 5.0f;
    }
    if (*(int *)(g + 0x2084) == 0x81) {
        col = 0xA0A0A0;
        s = 1.037f;
        a[2] = a[2] - *(float *)&D_0015EE6C_g * 3.0f;
    }
    p = func_L00_0026DEA0(b, 6, a, col, 0.05f, 1.01f, s, 30000.0f);
    if (p) ((char *)p)[3] = 0x44;
    if (*(int *)(data + 0xC) != 0) {
        d = func_001F9D10(data + 0x10, b) * 0.16667f;
        func_001F9BF0(c, data + 0x10, b);
        n = 4;
        if (*(int *)(g + 0x2084) == 8) n = 2;
        if (*(int *)(g + 0x2084) == 0x81) n = 1;
        for (i = 1; i < n; i++) {
            func_L00_001FF4B0(e, c, d * i);
            func_001F9BD8(e, b, e);
            p = func_L00_0026DEA0(e, 6, a, col, 0.05f, 1.01f, s, 30000.0f);
            if (p) ((char *)p)[3] = 0x44;
        }
    }
    qcopy(data + 0x10, b);
    *(int *)(data + 0xC) = 1;
}
