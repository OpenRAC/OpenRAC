#include "include_asm.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;
typedef float f32;

extern f32 func_00282C48(f32 t);
extern f32 func_00282C10(f32 t);

/* Clamps the measured value at P into the band the two arguments describe: the value is
   pulled down to the square root of twice the radius times the rate when it lies above it,
   and up to the opposite bound when it lies below.  The division is on its own line: the
   unit carries -g, so the assembler's division padding is reproduced. */
void func_L16_00317C90(f32 *p, f32 a1, f32 a2)
{
    f32 v = *p;
    f32 t;
    f32 r;
    f32 old;

    if (v * a1 <= 0.0f) {
        return;
    }
    if (a1 == 0.0f) {
        return;
    }
    t = (v * v / a2) * 0.5f;
    if (!(func_00282C48(a1) < t)) {
        return;
    }
    r = func_00282C10((a2 + a2) * a1);
    old = *p;
    if (r < old) {
        *p = r;
    } else {
        r = -r;
        if (old < r) {
            *p = r;
        }
    }
}
