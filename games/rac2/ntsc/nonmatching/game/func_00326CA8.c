/* func_00326CA8 - 12 bytes, 1 place (the boot), retail at 0x00326CA8:
   daddu $v0, $a1, $zero ; nop ; daddu $v0, $zero, $zero.
   Not a C row: catalogue defect.  The 12-byte extent is two 64-bit moves and a nop, with no
   `jr $ra`, between the previous function's `jr $ra` (func_00326BD8, 208 bytes, ends at
   0x00326CA8) and the next function's prologue (func_00326CB8 at 0x00326CB8); the same shape
   as func_0032FDC0 and the 4-byte rows below it.  The extent cannot be a function: it has no
   return, and the first move only copies a register that the extent never sets.
   No C can be placed - a fix in tools/levels.py that gives the twelve bytes to the
   neighbouring body would. */
