#include "include_asm.h"

typedef unsigned char u8;

/* The record the level allocates: the first quadword copy lands at +0x10, the
   second in the sub-object the +0x68 field points to. */
typedef struct NODE {
    char pad0[0x8];
    int f08;
    int f0C;
    char pad1[0x24 - 0x10];
    int f24;
    int f28;
    int f2C;
    unsigned char f30;
    char pad2[0x40 - 0x31];
    int f40;
    char pad3[0x68 - 0x44];
    void *f68;
    char pad4[0xB8 - 0x6C];
    int fB8;
    unsigned char fBC;
} NODE;

/* The quadword copy profile (-frac2-qcopy -frac2-qcopy-barrier) keeps both
   addresses in registers and groups the lq/sq pairs, as the retail does. */
typedef int TI128 __attribute__((mode(TI)));

extern NODE *func_0029EFD0(int);

extern void func_002A7750(NODE *, unsigned char, int);

/* Allocates the record, copies the two sixteen-byte blocks into it (the first at
   +0x10, the second in the sub-object its +0x68 field points to), fills its
   three fields and calls the level's helper. */
void *func_L00_003B7410(char *a0, void *a1, void *a2, void *a3)
{
    NODE *o = func_0029EFD0(0x88D);

    if (o != 0) {
        TI128 *d = (TI128 *)((char *)o + 0x10);
        TI128 *s = (TI128 *)o->f68;

        o->fB8 = (int)a0;
        o->f30 = 0xFF;
        o->fBC = (unsigned char)a3;
        *d = *(TI128 *)a1;
        *s = *(TI128 *)a2;
        func_002A7750(o, 2, -1);
    }
    return o;
}
