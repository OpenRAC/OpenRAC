#include "include_asm.h"

/* func_L07_003E6E00 copies a 16-byte vector, which needs the grouped quadword
   copy: it is in this sibling file, and config/cflags.txt gives it the
   -frac2-qcopy flag. */

/* The 16-byte vector the object and the entity keep. */
typedef int TI128 __attribute__((mode(TI)));

extern void func_00282948(char *);

extern void func_002B0990(char *, char *, int, int, float);

extern int D_L07_001C9780;

extern void func_L07_003E6FE8(char *);

extern void func_00325DA0(char *, char *);

extern void func_00282CF0(void *, void *, void *);

extern float func_00283B98(float, float);

extern void func_00325C50(char *, char *, int, void *, float);

/* Clear the two resource slots, step the state counter, refresh the object's
   transform and hand the entity to the shared launcher. */
void func_L07_003E6E00(char *arg)
{
    char *o;
    char *src;
    char *dst;
    TI128 v;
    unsigned short c;
    float f;

    o = *(char **)(arg + 0x68);
    func_00282948(o + 0x344);
    func_00282948(o + 0x348);
    c = *(unsigned short *)(arg + 0x96);
    if (c <= 0xFFFE)
        *(unsigned short *)(arg + 0x96) = c + 1;
    func_00325DA0(arg, o + 0x340);
    src = arg + 0x10;
    dst = o + 0x2E0;
    func_00282CF0(&v, src, dst);
    *(TI128 *)dst = *(TI128 *)src;
    if (D_L07_001C9780 != 0)
        func_L07_003E6FE8(arg);
    if (*(unsigned char *)(arg + 0x95) == 2) {
        f = func_00283B98(*(float *)(o + 0x2DC), 0.20943954586982727f);
        *(float *)(o + 0x2DC) = f;
        func_00325C50(arg, o + 0x330, 1, &v, f);
    } else {
        f = func_00283B98(*(float *)(o + 0x2DC), 0.011635531f);
        *(float *)(o + 0x2DC) = f;
        func_00325C50(arg, o + 0x330, 0, &v, f);
    }
    func_002B0990(arg, o + 0x230, 7, 1, 2.5f);
}
