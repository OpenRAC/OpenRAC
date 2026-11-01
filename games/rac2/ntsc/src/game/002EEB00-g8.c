#include "include_asm.h"

extern char *D_L00_001B24FC;
extern char *D_L00_001B2608;
extern int D_L00_001B2504;
extern int D_001A72D0 __attribute__((sda));
extern char D_L00_001AD3D0[];

extern void func_002F00A8(void);
extern int func_002F0218(int);
extern void func_002FC748(void);
extern void func_0026FEB8(const char *, ...);

void func_002EF1E8(void)
{
    char *p;
    char *q;
    int chain;

    p = D_L00_001B24FC;
    q = D_L00_001B2608;
    D_L00_001B2608 = q + 16;
    *(int *)(p + 0) = 0x20000000;
    *(char **)(D_L00_001B24FC + 4) = D_L00_001B2608;
    *(int *)(D_L00_001B24FC + 8) = 0;
    *(int *)(D_L00_001B24FC + 12) = 0;
    func_002F00A8();
    chain = func_002F0218(D_001A72D0);
    func_002FC748();
    if (0x400000 < chain) {
        func_0026FEB8(D_L00_001AD3D0);
    }
    if (D_L00_001B2504 < chain) {
        D_L00_001B2504 = chain;
    }

    *(int *)(D_L00_001B2608 + 0) = 0x20000000;
    *(char **)(D_L00_001B2608 + 4) = D_L00_001B24FC + 16;
    *(int *)(D_L00_001B2608 + 8) = 0;
    *(int *)(D_L00_001B2608 + 12) = 0;
    D_L00_001B2608 = D_L00_001B2608 + 16;
    *(int *)(q + 0) = 0x20000000;
    *(char **)(q + 4) = D_L00_001B2608;
    *(int *)(q + 8) = 0;
    *(int *)(q + 12) = 0;
}
