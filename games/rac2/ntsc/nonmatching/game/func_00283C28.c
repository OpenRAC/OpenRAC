/* func_00283C28 -- an angle wrapped to [-pi, pi], 88 bytes in retail.

   Retail: the fraction of a turn is taken out of the angle and what is left
   is scaled back to radians.

   Measured with tools/check.py in src/game/0027F128.c (the -O2 -G0 profile):
   "func_00283C28: 0 of 28 places match", 92 bytes against retail's 88.

   What differs, after this wave's measurements:

   - the branchless floor is now reproduced: a union of a float and an int
     gives the float's sign bit the way retail does (`mfc1` of the float's
     bits, shifted right by 31), and the candidate is four bytes over
     because this cc1 reads the union's word as 64 bits first
     (`daddu $v0,$a0,$zero`) and shifts that, where retail shifts the word
     in place (`sra $at,$at,31`).
   - the final scale is pi (0x40490FDB) in retail, and the candidate's
     constant lands in `$f0` where retail keeps it in `$f1`; the working
     value stays in `$f1`/`$f12` where retail keeps it in the return
     register `$f0`.
   - retail adds the sign to the integer with the trapping `add`
     (`add $at,$at,$v0`); this cc1 uses `addu` for every 32-bit addition
     (measured with the compiler itself, tools/probe3.c), so that word is
     out of reach.

   A dedicated `-ftrapv` file was also rejected by this cc1 as an invalid
   option, so it cannot be used to request the trapping `add`.

   The trapping `add` and the 64-bit union read are what is left.  The
   trapping `add` is the mark the repository already uses for handwritten
   code: func_00283150, in the same file, is marked ASM_FUNC and its body is
   `add $v0,$v0,$a2` / `add $v0,$v0,$a3` with the same trapping additions.
*/
typedef float f32;
typedef int s32;

typedef union { f32 f; s32 i; } FloatInt;

f32 func_00283C28(f32 a0) {
    f32 t = a0 * 0.15915494f + 0.5f;
    FloatInt u;
    s32 i;

    u.f = t;
    i = (s32)t;
    i += u.i >> 31;
    return (t - (f32)i - 0.5f) * 3.1415927f;
}
