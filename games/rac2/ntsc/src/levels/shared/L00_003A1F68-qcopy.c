#include "include_asm.h"
typedef int TI128 __attribute__((mode(TI)));
extern void func_00282CF0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
void func_L00_003A1F68(TI128 *out, TI128 *first, TI128 *second, float t)
{
    TI128 local;
    if (t == 0.0f) {
        *out = *first;
    } else if (t == 1.0f) {
        *out = *second;
    } else {
        func_00282CF0(&local, second, first);
        func_00282D30(&local, &local, t);
        func_00282CC0(out, first, &local);
    }
}
