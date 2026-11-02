/* Carried from rac3-decomp (src/frontbin/3AA9D8.c, func_003AACC0): the same instructions.
   Writes func_0034EE78(t, 0, p[8], p[0xC], 1) to *out. */

#include "common.h"

extern f32 func_0034EE78(f32, f32, f32, f32, f32);

void func_00335C10(u8 *p, f32 *out, f32 t) {
    *out = func_0034EE78(t, 0.0f, *(f32 *)(p + 8), *(f32 *)(p + 0xC), 1.0f);
}
