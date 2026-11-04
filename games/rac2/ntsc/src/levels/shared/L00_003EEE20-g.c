#include "include_asm.h"


typedef int s32;
typedef float f32;
extern f32 func_00283CE0(s32);
extern int func_00283CF0(f32);

/* Same body as func_L00_00329FF0, with this function's own callees. */
s32 func_L00_003EEE20(s32 value) {
    s32 divisor = 16;

    if (value % divisor != 0) {
        f32 q;
        f32 d;

        q = func_00283CE0(value);
        d = (f32)divisor;
        q = q / d;
        value = (func_00283CF0(q) + 1) << 4;
    }
    return value;
}
