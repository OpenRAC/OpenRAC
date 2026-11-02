/* No attempt: the row starts inside the previous body. 288 bytes at 0x35E890
   begin

     0035E890  sw    $t3, 0x40($v1)      <- $v1 is never set in the row
     0035E894  nop
     0035E898  addiu $sp, $sp, 0x20     <- the previous frame's restore
     0035E89C  nop

   which is the tail of the body that ends with `jr $ra` at 0x35E884 and its
   delay slot `addiu $sp,$sp,0x50` at 0x35E888.  The real body starts at
   0x35E8A0 (`lui $a1,0x14`; `daddu $t2,$a0,$zero`; ...) and is 272 bytes
   long: the next row, func_002DBAA8, is at 0x35E9B0.

   Measured: a scan of every branch, jump and call target and of every
   aligned word in the loaded segments of the level 13 program finds no
   reference to 0x35E890, while 0x35E8A0 has an aligned data pointer at
   0x291C90.  A tools/levels.py split is what unblocks it. */
