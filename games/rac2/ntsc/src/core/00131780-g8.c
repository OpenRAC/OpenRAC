#include "common.h"

extern int r1_D_0015EDC4 __attribute__((sda));
extern void func_00132028(void);

void func_00133230(void) {
    r1_D_0015EDC4 = 0;
    func_00132028();
}

extern int D_001A74F8 __attribute__((sda));
extern char D_001A7578[];
extern void func_00115D48(const void *, ...);
extern void func_0011AEA0(int);
extern int func_0011D810(const void *);
extern int func_0011D620(const void *, int, int, int *, int, int *, int, int, int);
extern int *D_001A74A0[2] __attribute__((sda));
extern int D_001A74A8[2] __attribute__((sda));
extern int *D_001A74B8[2] __attribute__((sda));
extern int D_001A74C0 __attribute__((sda));
extern int D_001A7040;
extern void func_001322B8(void *, int, void *);
void func_00133108(void) {
    func_001322B8(D_001A74B8[D_001A74C0], *D_001A74A0[D_001A74C0], D_001A74A0[D_001A74C0]);
    while (func_0011D810(&D_001A7040)) {
        if (D_001A74F8 == 0)
            func_00115D48(D_001A7578);
        func_0011AEA0(0);
    }
    func_0011D620(&D_001A7040, 0x4D, 1, D_001A74A0[D_001A74C0], 0x1000-D_001A74A8[D_001A74C0], D_001A74B8[D_001A74C0], (*D_001A74A0[D_001A74C0] << 2)+8, 0, 0);
    {
        int next = (D_001A74C0 ^ 1) != 0;
        D_001A74C0 = next;
        *D_001A74A0[next] = 0;
        D_001A74A8[next] = 0xFFC;
    }
}
