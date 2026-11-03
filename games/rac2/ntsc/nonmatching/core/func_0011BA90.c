/* Not a function: the row's twelve bytes are

 *     0x0011BA90  addiu  $sp, $sp, 0x20
 *     0x0011BA94  nop
 *     0x0011BA98  addiu  $sp, $sp, 0x20

 * There is no jr $ra, and no function begins by restoring a frame it never
 * allocated: the row is a pair of stripped epilogues.
 *
 * The previous row, func_0011BA58, ends cleanly at 0x0011BA8C - `jr $ra` at
 * 0x0011BA84 and the matching `addiu $sp,$sp,0x20` in its delay slot at
 * 0x0011BA88, with a nop of padding at 0x0011BA8C - and the row holds two
 * more copies of the same restore.  The next row, func_0011BAA0, starts its
 * frame at 0x0011BAA0.
 *
 * tools/check.py with `void func_0011BA90(void) {}` gives 8 bytes against the
 * row's 12.  A tools/levels.py split removes the row; there is no C body to
 * find. */
