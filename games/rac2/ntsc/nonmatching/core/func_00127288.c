/* Not a C row: a linker remnant, not a function. The 28 bytes at 0x00127288 are
 * `addiu $sp,$sp,0x40; nop; addiu $sp,$sp,0x40; nop; addiu $sp,$sp,0x40; nop;
 * addiu $sp,$sp,0x10` - no `jr $ra`, no branch, no register traffic, and the row
 * opens on a stack *restore*, which no prologue can be. It is the 32-byte gap
 * between func_00127218 (whose epilogue `jr $ra; addiu $sp,$sp,0x30` ends at
 * 0x00127284) and func_001272A8 at 0x001272A8; the row takes 28 of those bytes,
 * and the `nop` at 0x001272A4 belongs to none. Nothing in the boot jumps to
 * 0x00127288.
 *
 * tools/check.py func_00127288 --diff: a candidate of the only shape that could
 * reach those bytes (three 0x40-byte volatile locals) compiles to 28 bytes and
 * prints "boot: instruction at +0x0 differs" - the retail words are the three
 * restore pairs against the candidate's `addiu $sp,$sp,-0xC0` prologue - and
 * "0 of 1 places match". No C starts a function with a stack restore.
 *
 * Same class as func_00127218 next door and func_00127340 after it: a split
 * between two entries, fixed in tools/levels.py, not a C body. */
