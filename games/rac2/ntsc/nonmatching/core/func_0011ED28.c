/* func_0011ED28 (56 bytes, boot 0x0011ED28) is not a function, so there is
   no C for it.  The catalogue's extent covers the last 52 bytes of the SDK
   object that starts at 0x0011ED08 (func_0011ED08) plus the linker's
   0xCDCDCDCD fill word that ends the object at 0x0011ED5F.

   Those 52 bytes are thirteen instructions and nothing else:

       addiu $sp, $sp, 0x10 / nop
       addiu $sp, $sp, 0x80 / nop
       addiu $sp, $sp, 0x10 / nop
       addiu $sp, $sp, 0x70 / nop
       addiu $sp, $sp, 0x10 / nop
       addiu $sp, $sp, 0x60 / nop
       addiu $sp, $sp, 0x50

   a bare run of stack-pointer adjustments (0x1D0 bytes in total), with no
   `jr $ra` anywhere in the 56 bytes and no branch at all.  Nothing in the
   boot refers to 0x0011ED28 (no `jal`, no `j`, and no address half pair),
   and the next function (func_0011ED60) starts with its own prologue
   (`addiu $sp, $sp, -0x40`).

   The same shape appears elsewhere in the boot's text - at 0x0011E740
   (twelve pairs, between func_0011E4E0's `jr $ra` epilogue and
   func_0011E740's own `addiu $sp, $sp, -0x30`), at 0x002EDA28 (six pairs,
   before the `addiu $sp, $sp, -0x160` of the function at 0x002EDA58, which
   saves $f20-$f23) and at 0x00281E88, 0x0028BC50 and others - always
   between a finished function's epilogue and the next function's prologue.

   Neither the SDK's EE-GCC nor the game's compiler emits such a run for
   any C: a frame of that size comes out as a single `subu $sp, $sp, N`
   (and `addu $sp, $sp, N` for its release), and a function always ends in
   `jr $ra`.  A run like this is a frame-release sequence of its own, not
   a function.

   So the extent is not a function of its own: the catalogue gave a name
   to a piece of the neighbouring code.  That is a fix in tools/levels.py,
   not in C - see AGENTS.md, "When to stop on a function". */
