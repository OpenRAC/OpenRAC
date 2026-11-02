/* No attempt: the row is not a function. 20 bytes at 0x0011D850 are
   `addiu $sp,$sp,0x30; nop; addiu $sp,$sp,0x40; nop; addiu $sp,$sp,0x80`:
   three stack releases, no `jr $ra`, no register traffic. The compiler emits
   one frame adjustment and a return, so no C reaches it.
   The block stands alone between two functions: func_0011D810
   (sceSifCheckStatRpc) ends with `jr $ra` at 0x0011D844, and the next
   function starts at 0x0011D868 with its own prologue.
   rac1-decomp keeps the same bytes as the linker remnant func_0011B6F8
   (LINKER_REMNANT; the sifrpc object's address delta is +0x2158, so rac1
   0x0011B6F8 is rac2 0x0011D850), and the USA v2.00 symbol table has no
   symbol at 0x0011D850 while it has sceSifCheckStatRpc at 0x0011D810.
   Two C spellings measured with tools/check.py --diff: an empty function is
   8 bytes against retail's 20, one with a 0xF0-byte frame is 16. A
   split-off remnant - fix in tools/levels.py. */
