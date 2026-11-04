/* func_002AD0E0 in its own unit: its early-out copies 128 bits through lq/sq
   with both addresses in registers, which is the compiler's opt-in grouped
   copy (config/cflags.txt gives this file -frac2-qcopy), so the unit holds
   this function alone. */

#include "include_asm.h"

typedef int TI128 __attribute__((mode(TI)));

typedef struct { char pad[64]; } Matrix64_002AD0E0;

extern int func_002AD0B0(unsigned char *a);
extern void func_00283410(char *dst, char *src);
extern void func_00283698(char *dst, char *src);
extern void func_00283098(char *a, char *b, char *c);
extern void func_00282CC0(char *dst, char *a, char *b);
extern void func_00282CF0(char *dst, char *a, char *b);
extern void func_00283788(char *a, char *b, char *c);
extern void func_002AB6D8(char *a, char *b);

/* Places the position a2 into a4 relative to the object a1 and its matrix
   target, and a3 into a5; without the object's target both are plain copies. */
int func_002AD0E0(void *unused, void *a1, void *a2, void *a3, void *a4, void *a5)
{
    Matrix64_002AD0E0 m0, m1, m2;
    char *p = (char *)func_002AD0B0((unsigned char *)a1);

    if (p == 0) {
        *(TI128 *)a4 = *(TI128 *)a2;
        *(TI128 *)a5 = *(TI128 *)a3;
        return 0;
    }
    func_00283410((char *)&m0, p);
    func_00282CC0((char *)a4, (char *)a2, p + 0x10);
    func_00282CF0((char *)a4, (char *)a4, (char *)a1 + 0x10);
    if (*(int *)(p + 0x3C) & 2) {
        func_00283410((char *)&m2, p + 0x20);
        func_00283698((char *)&m1, (char *)&m2);
        func_00283098((char *)a4, (char *)a4, (char *)&m1);
        func_00283098((char *)a4, (char *)a4, (char *)a1 + 0xC0);
    } else {
        func_00283098((char *)a4, (char *)a4, (char *)&m0);
    }
    func_00282CC0((char *)a4, (char *)a4, (char *)a1 + 0x10);
    func_00283410((char *)&m1, (char *)a3);
    func_00283788((char *)&m1, (char *)&m0, (char *)&m1);
    func_002AB6D8((char *)&m1, (char *)a5);
    return 1;
}
