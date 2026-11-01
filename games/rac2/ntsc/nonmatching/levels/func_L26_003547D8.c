/* func_L26_003547D8 (288 bytes, 1 placement, level 26 jamming array): CATALOGUE DEFECT.
   The extent is not a function: it starts inside the preceding function's epilogue.
   Level 26 bytes at 0x003547D8..0x003547E8 are

       sw    $t3, 0x40($v1)
       nop
       addiu $sp, $sp, 0x20
       nop

   with no prologue, no register ever set and no `jr $ra`: the tail of a body that
   belongs to the previous function. The previous entry, the level-26 copy of the boot
   function func_002DB7F0, runs 0x00354640..0x003547D3 and ends with a complete
   `jr $ra` (0x003547CC) and its delay slot `addiu $sp,$sp,0x50` (0x003547D0), followed by
   one pad word (0x003547D4); the next entry opens at 0x003547E8 with a real body
   (`lui $a1,0x14; daddu $t2,$a0,$zero; lw $v0,-0x66B8($a1)`), so the boundary is at
   0x003547E8 and the catalogue's start is 0x10 bytes too early.

   This is the same defect the catalogue already corrects for level 00, where the entry
   00354DE8 is moved to 00354DF8 ("tools/levels.py corrects three measured Aranos entries
   before fingerprinting ... These corrections apply to level 00 only", docs/LEVELS.md).
   The level-26 instance of the same body is 0x003547D8 (level 0 + 0x2AF0) and carries the
   same sixteen bytes: level 0 at 0x00354DE8 reads `sw $t3,0x40($v1); nop;
   addiu $sp,$sp,0x20; nop` and the body starts at 0x00354DF8.

   No C can be placed on the extent: the sixteen bytes are not a function entry. The fix is
   in tools/levels.py (the same correction for every level), not in C. */

#include "include_asm.h"
