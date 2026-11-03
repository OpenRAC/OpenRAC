/* No attempt: the retail bytes are not a function.  The 12-byte extent of
   func_0032FDC0 is daddu $v0, $a1, $a2 ; nop ; daddu $v0, $zero, $zero ; nop - two 64-bit moves and no `jr $ra` - with no prologue, no
   body and no `jr $ra`, and it sits between the previous function's `jr $ra`
   and its epilogue and the next function's prologue.  It is a catalogue split
   error, not code: no C can be placed there.  The same shape as
   func_L00_00341798 and func_L00_002E6440. */
