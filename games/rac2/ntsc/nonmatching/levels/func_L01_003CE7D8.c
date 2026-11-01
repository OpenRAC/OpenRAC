/* 0 of 1 place match - last line: 01_oozla: 152 bytes, expected 144
   Six float arguments (four FP registers) and two pointers: scales *a1 by (1.0f - the fifth
   argument), squared, then blends it with *a0 - the third argument and the sixth argument,
   divides by the sixth argument squared, and leaves *a0 as the running total while the first
   argument goes to func_00282C48.
   Retail, word for word: `mul.s $f15,$f15,$f15` (r*r) ; `lwc1 $f1,0x0($a1)` ; the 1.0f - a
   fold, which is THREE operations in retail - `sub.s $f14,$f0,$f14` then
   `sub.s $f14,$f14,$f0` then `add.s $f14,$f14,$f0` - so the source carries a dead
   `(x - 1.0f) + 1.0f` that strict IEEE arithmetic forbids folding ; `mul.s $f1,$f1,$f14` and
   `swc1` ; then `lwc1 $f2,0x0($a0)`, `sub.s $f0,$f2,$f12`, `mul.s` (so *a0 is loaded once into
   $f2 and re-read by the sign test) ; `c.olt.s $f15,$f0` and the conditional is the branch-likely
   pair `bc1tl -> mul.s` with `mov.s $f0,$f15` in the slot, i.e. the then-arm jumps OVER the
   `mul.s $f0,$f0,$f13` ; `c.olt.s $f12,$f2` then `bc1fl` with `add.s $f0,$f1,$f0` in the slot
   and `sub.s $f0,$f1,$f0` as the fall-through.
   Measured once from a straightforward C: 152 bytes against 144. This cc1 loads *a0 before *a1
   (retail loads *a1 first), re-uses $f3 instead of $f2 for the first *a0 value, and - the size
   difference - needs $f20 and a 0x20 frame to keep the blend across the `div.s`, where retail
   needs no callee-saved FP register at all; it also emits `bc1f` + an explicit `b` where retail
   inverts to `bc1tl`.
   What unblocks it: a spelling that stops $f20 being allocated (the whole body then fits the
   0x10 frame) and gets the branch inverted. */

/* Near miss: 140 against 144 bytes, one word short.  Level 01, 0x003CE7D8.

   Everything up to the float branch is retail's, instruction for instruction:
   the `lui $at,0x3F80 ; mtc1 $at,$f0`, the `mul.s $f15,$f15,$f15`, the
   three `f14` round trips, the `c.olt.s $f15,$f0` + `bc1tl` with
   `mov.s $f0,$f15` in the delay slot, the `mul.s $f0,$f0,$f13` and the two
   `lwc1`/`swc1` tails.

   What differs:

   1. the division.  Retail has `c.olt.s $f12,$f2 ; nop ; nop ;
      div.s $f0,$f0,$f15 ; bc1fl ...` - two nops between the compare and the
      divide - and this candidate has none, so it is four bytes short.  The
      project's assembler pads a division only when it lands fewer than two
      slots after a code label (tools/compiler/transformers/
      pad_div_erratum_nops.py) or fewer than two slots after the end of a
      branch group (pad_div_erratum_branch.py); with the division two slots
      after the `c.olt.s` neither fires.  The retail's two nops are not
      reproduced by any source shape measured here.

   2. the `bc1fl` delay slot.  Retail fills it with the first instruction of
      the branch's own block (`add.s $f0,$f1,$f0`); this candidate puts the
      `swc1 $f0,0($a1)` there and keeps a second copy of the store after the
      block, because the store's value depends on the branch.

   The float branch-likely forms are the ones measured in
   nonmatching/levels/func_L20_002C7588.c: `if (!(a > b)) { } else { x = b; }`
   emits `c.lt.s b,a` + `bc1tl` and the plain `if (a < b)` emits
   `c.lt.s a,b` + `bc1fl`.  Both are used here.

   The `func_002E33E0` of the retail is func_00282C48 (level 01, 0x002E33E0),
   an eight-byte boot function. */


typedef float f32;

extern float func_00282C48(float a0);

float func_L01_003CE7D8(f32 *a0, f32 *a1, f32 f12, f32 f13, f32 f14, f32 f15)
{
    f32 f0, f1, f2;

    f0 = 1.0f;
    f15 = f15 * f15;
    f1 = *a1;
    f14 = 1.0f - f14;
    f14 = f14 - 1.0f;
    f14 = f14 + 1.0f;
    f1 = f1 * f14;
    *a1 = f1;
    f2 = *a0;
    f0 = f2 - f12;
    f0 = f0 * f0;
    if (!(f0 > f15)) {
    } else {
        f0 = f15;
    }
    f0 = f0 * f13;
    f0 = f0 / f15;
    if (f12 < f2)
        f0 = f1 - (f1 + f0);
    *a1 = f0;
    f1 = *a1;
    f0 = *a0;
    f0 = f0 + f1;
    f12 = f12 - f0;
    *a0 = f0;
    return func_00282C48(f12);
}
