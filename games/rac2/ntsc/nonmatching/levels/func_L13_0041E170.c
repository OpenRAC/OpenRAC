/* No attempt: the row is not a function. 4 bytes at 0x41E170 hold one word,
   `sltiu $v0,$v0,0x1` (0x2C420001), which reads $v0 - a register the extent
   never sets.

   The previous body, func_L12_0041A578 (108 bytes at 0x41E100), ends with
   `jr $ra` at 0x41E164 and its delay slot `daddu $v0,$a1,$zero` at 0x41E168;
   0x41E16C is a nop of alignment padding.  The next body,
   func_L12_0041A6D0, starts at 0x41E178.  The word at 0x41E170 is the
   residue the compiler left after that return, one nop later than the
   four-byte residue rule of tools/levels.py accepts (that rule needs the
   extent to start immediately after the delay slot).

   Measured: a scan of every branch, jump and call target and of every
   aligned word in the loaded segments of the level 13 program finds no
   reference to 0x41E170, while 0x41E178 (the next body) is referenced.  A
   tools/levels.py split is what unblocks it. */
