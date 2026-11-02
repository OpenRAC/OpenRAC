#include "include_asm.h"

/* func_L00_0037FA90 is a unit of its own: the 16-byte constant the level copies into
   the local vector needs the opt-in grouped quadword-copy profile (-frac2-qcopy,
   config/cflags.txt), which gives retail's `lui $v1 ; addiu $v1 ; lq $v0,0($v1) ;
   sq $v0,0($sp)`. The default profile keeps the address in $v0 and the quadword in
   $v1, the two registers swapped.

   The block is built from the level's stored vectors, placed, and the reach against
   the level's limit decides the near or the far branch. */

typedef int L00_0037FA90_TI128 __attribute__((mode(TI)));

extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern int func_002761F8(void *, void *, int, void *, int);
extern void func_00282CF0(void *, void *, void *);
extern float func_00282DB0(void *, void *);
extern float func_00283180(float);
extern void func_00282DD8(void *, void *, void *);
extern void func_002ACEC0(void *, void *, void *, float);
extern void func_00282D30(void *, void *, float);

extern L00_0037FA90_TI128 D_L00_001BF220;
/* The input at +0x230 and the lower vector share this measured data block. */
extern char D_L00_001B8DC0[0x240];

/* The level's own data: the vector block the reach is measured against and the
   pointer the shared test takes. */
typedef struct {
    unsigned char pad000[0x760];
    unsigned char f760[0x1b30];
    void *f2290;
} L00_0037FA90_Data;

extern L00_0037FA90_Data D_00189E20;

void func_L00_0037FA90(char *a0, char *a1, float f12, float f13, float f14)
{
    char *p = D_L00_001B8DC0 + 0x230;
    char local[16];
    char local2[16];
    float reach;

    func_00282F20(local, p, f13);
    p -= 0x230;
    func_00282CC0(local, local, p);
    if (func_002761F8(p, local, 0, D_00189E20.f2290, 0) != 0)
        *(L00_0037FA90_TI128 *)local = D_L00_001BF220;
    func_00282CF0(a0, local, a1);
    func_00282F20(a0, a0, 1.0f);
    reach = func_00282DB0(a0, D_00189E20.f760);
    if (reach < func_00283180(f14)) {
        func_00282DD8(local2, a0, D_00189E20.f760);
        func_002ACEC0(a0, D_00189E20.f760, local2, f14);
        func_00282F20(a0, a0, f12);
    } else {
        func_00282D30(a0, a0, f12);
    }
}
