/* func_00281E88, 44 bytes, 1 placement (the boot).
   Not a C row: catalogue defect.  The extent at 0x00281E88 in the boot is
   five `addiu $sp,$sp,N` pairs with nops - 0x30, 0x80, 0xC0, 0xF0, 0x130 and
   a last `addiu $sp,$sp,0x220` - with no `jr $ra` anywhere in the row and
   no branch out of it: it is linker fill, the tail of a neighbour's frame
   (the same shape as func_00282848 in the levels).  No C can be placed
   there: one C function cannot be five `addiu $sp,$sp,N`.  A
   `tools/levels.py` split is what unblocks it. */

#include "include_asm.h"
