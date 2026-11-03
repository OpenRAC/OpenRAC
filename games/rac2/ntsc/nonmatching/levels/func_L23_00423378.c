/* No attempt: the row is not a function. 4 bytes at 0x423378 hold one word,
   `daddu $v0,$zero,$zero` (0x0000102D) - the compiler's `move $v0,$zero`,
   the return value of a function that already returned.

   The previous body ends with `jr $ra` at 0x423364 and its delay slot
   `addiu $sp,$sp,0x160` at 0x423368; 0x42336C, 0x423370 and 0x423374 are
   nops of alignment padding.  The next body, func_L23_00423380, starts at
   0x423380.

   Measured: a scan of every branch, jump and call target and of every
   aligned word in the loaded segments of the level 23 program finds no
   reference to 0x423378.  A tools/levels.py split is what unblocks it. */
