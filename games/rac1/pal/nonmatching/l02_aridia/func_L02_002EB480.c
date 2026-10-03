/* NON_MATCHING func_L02_002EB480 -- src/overlays/l02_aridia/vendor_002E21F8.c
 * Best so far: BYTES 12/336 (96.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draw callback (AddDrawCallback(func_L02_002EB480, moby)): calls func_L02_002A59F8(15,15,25,30, 0.0f, 260080.0f
 *   p4.c: 12/336 bytes differ, only scheduling of the argument setup for the first call (retail puts mov.s $f15,$f
 *   Landing caveats: the file declares func_L02_002EB480(void) after it, and func_L02_002A59D8(float) which the as
 */
#include "common.h"
extern int D_L02_0015F6E8 MACRO_ADDR;
extern char D_L02_001F4140[];
extern char D_L02_001F3EC0[];
extern char D_L02_00167440[];
extern char D_L02_001F3F90[];
extern char D_L02_001F3FF0[];
extern char D_L02_001F4050[];
extern char D_L02_001F4090[];
extern char D_L02_001F40D0[];
extern char D_L02_001F3F20[];
extern void func_L02_002A59F8(int a, int b, int c, int d, float x, float y, float z, float w, int e, int f, int g);
extern int func_00215570(void *, int);
extern void func_00202790(int);
extern void func_L02_002A59D8_x() __asm__("func_L02_002A59D8");

extern void func_L02_002EB480_m(char *) __asm__("func_L02_002EB480");

/* draw callback: set up the lights for the enabled slots */
void func_L02_002EB480_m(char *moby) {
    float z = 0.0f;
    float y = 260080.0f;
    float u = 255.0f;
    int *p = *(int **)(moby + 0x78);
    func_L02_002A59F8(0xF, 0xF, 0x19, 0x1E, z, y, u, z, 0x28, 0x40, (int)D_L02_001F4140);
    if (D_L02_0015F6E8 != 0 || (p[0] != -1 && func_00215570(D_L02_00167440, p[0]))) {
        func_00202790((int)D_L02_001F3EC0);
    }
    if (D_L02_0015F6E8 != 0 || (p[1] != -1 && func_00215570(D_L02_00167440, p[1]))) {
        func_00202790((int)D_L02_001F3F90);
        func_00202790((int)D_L02_001F3FF0);
        func_00202790((int)D_L02_001F4050);
        func_00202790((int)D_L02_001F4090);
        func_00202790((int)D_L02_001F40D0);
    }
    if (D_L02_0015F6E8 != 0 || (p[2] != -1 && func_00215570(D_L02_00167440, p[2]))) {
        func_00202790((int)D_L02_001F3F20);
    }
    func_L02_002A59D8_x();
}
