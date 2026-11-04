/* func_L00_003D1B48 - 4 bytes, 27 places, retail at 0x003D1B48 (level 00):
   slt $v0, $a0, $v0 - the same word in every one of the 27 levels.
   Not a C row: catalogue defect.  The 4-byte extent is one comparison that reads $v0
   without setting it and has no `jr $ra`, in the padding gap between the previous
   function's end (func_003202E8, 612 bytes, ends at 0x003D1B44 in level 00) and the next
   function's prologue (func_L00_003D1B50 at 0x003D1B50); the same shape as func_0032F590.
   The extent cannot be a function: it never returns and it reads a register it never sets.
   No C can be placed - a fix in tools/levels.py that gives the four bytes to the
   neighbouring body would. */
