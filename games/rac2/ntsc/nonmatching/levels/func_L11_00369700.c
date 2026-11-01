/* No attempt: the retail bytes at the catalogue's start are not the function's
   first code. The extent starts at 0x00369700 with `sw $t3, 0x40($v1)`
   (40006BAC) and a `nop` after it: $v1 is never set there, and the two words
   are the tail of the previous function, whose `jr $ra` is at 0x003696F4 with
   its epilogue `addiu $sp, $sp, 0x50` in the delay slot and the padding
   `nop` at 0x003696FC. The function itself starts at 0x00369708 with the
   prologue `addiu $sp, $sp, 0x20` and runs to the `jr $ra` at 0x00369818,
   280 bytes. splat marks 0x00369700 as an `alabel` (an alternate entry of
   the previous function), not a `glabel`. The catalogue row
   func_L11_00369700 (288 bytes at 11:00369700) is therefore eight bytes too
   long and starts eight bytes too early: the fix belongs in
   tools/levels.py, not in C. No C can be placed at the catalogue's extent. */
