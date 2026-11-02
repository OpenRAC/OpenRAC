/* No attempt: the retail bytes are not a function.  The 20-byte extent of
   func_00335420 is addiu $sp, $sp, 0x20 ; nop ; addiu $sp, $sp, 0x20 ; nop ; addiu $sp, $sp, 0x20 - three stack adjustments - with no prologue, no
   body and no `jr $ra`, and it sits between the previous function's `jr $ra`
   and its epilogue and the next function's prologue.  It is a catalogue split
   error, not code: no C can be placed there.  The same shape as
   func_L00_00341798 and func_L00_002E6440. */
