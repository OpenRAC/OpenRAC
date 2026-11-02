#include "include_asm.h"
extern char *D_L23_001B1484 __attribute__((sda));
extern int func_002E5CF8(int index, int a1, char *a2);
typedef int TI128 __attribute__((mode(TI)));
extern void func_00283098(char *, char *, char *);
extern void func_00282CC0(char *, char *, char *);
extern void func_0032C700(char *, int, int, int, int, int, int, int, float, float, int, int, int, int);
extern char D_00189E20[];
extern void func_00282CC0(char *, char *, char *);

/* Builds the measured vector when the state's first word names the argument, and lays out the call. */
void func_PAL_L23_004217B0(char *arg0) {
    union {
        TI128 q;
        float f[4];
    } vec;
    char *base = D_00189E20;

    if (*(char **)(base + 0x2290) == arg0) {
        vec.q = 0;
        vec.f[0] = 1.3f;
        vec.f[2] = 4.5f;
        vec.f[3] = 1.0f;
        func_00283098((char *)&vec, (char *)&vec, arg0 + 0xC0);
        func_00282CC0((char *)&vec, (char *)&vec, base + 0x80);
        func_0032C700((char *)&vec, 0, 10, 10, 10, 25, 30, 0, 3.0f, 20.0f, 0, 1, 0, 0);
    } else {
        func_0032C700(arg0 + 0x10, 0, 25, 20, 20, 15, 25, 1, 1.5f, 20.0f, 0, 0, 0, 0);
    }
    if (*(short *)(arg0 + 0xAA) != 0x118F)
        func_002E5CF8(3, 0, D_L23_001B1484);
}
