/* func_00283B98 -- the sum of two angles, wrapped to [-pi, pi], 72 bytes in
   retail.

   Retail: a0 + a1, then -2*pi when it is not below pi, and +2*pi when it is
   below -pi.

   Measured with tools/check.py in src/game/0027F128.c (the -O2 -G0 profile):
   "func_00283B98: 0 of 28 places match", 92 bytes against retail's 72.

   What differs, after this wave's measurements:

   - the shape is now right: writing the two returns separately (`if (r < p)
     { ... return r; }` and the subtract arm after it) gives retail's compare
     (`c.olt.s`) and its `bc1t` over the subtract arm, and the two separate
     `r + p` / `r - p` statements keep retail's two `add.s` / two `sub.s` of
     pi - this cc1 folds a literal `pi + pi` into the constant 0x40C90FDB.
   - the residue is the float register allocation: retail keeps the sum in
     `$f0` (the return register), pi in `$f14` and -pi in `$f15`; this cc1
     keeps the sum in `$f12` and the two constants in `$f1` and `$f0`, and
     needs one more branch plus a `mov.s` in the return delay slot.
   - the constants land in the third and fourth float argument registers
     ($f14/$f15) when they are the arguments of a function that is inlined
     into its caller, and in `$f1`/`$f0` when they are locals of the function
     itself.  Adding unused float parameters to the prototype does not move
     them (measured).
   - the SDK compiler profile (build/compiler/sdk, "sdk -O2 -G0
     -ffunction-sections -fno-builtin") emits the same words as the game's
     compiler for this body.

   The float register allocation is what is left.
*/
typedef float f32;

f32 func_00283B98(f32 a0, f32 a1) {
    f32 r = a0 + a1;
    f32 p = 3.1415927f;
    f32 n = -3.1415927f;

    if (r < p) {
        if (r < n) {
            r = r + p;
            r = r + p;
        }
        return r;
    }
    r = r - p;
    r = r - p;
    return r;
}
