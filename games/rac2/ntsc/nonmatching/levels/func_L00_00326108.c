/* No attempt: the retail bytes are not code. The four-byte extent at
   0x00326108 is 0xA480001C - a bare `sh $zero, 0x1C($a0)` with no prologue, no body and no
   `jr $ra` - and the four bytes after it are a `nop`. It sits between the
   previous function's `jr $ra` and its epilogue and the next function's prologue `addiu $sp,$sp,-0x30`, at 0x00326110.
   The same shape as func_L00_00341798 and func_L00_002F4018: a catalogue
   split error, not a function. No C can be placed. */
