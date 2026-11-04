/* Not a C row: a linker remnant, not a function. The four bytes at 0x0011F710 are
 * `addiu $sp,$sp,0x20` - a stack restore with no `jr $ra` and no register
 * traffic, so no prologue can be. It is the first half of the eight-byte gap
 * between func_0011F700 (whose `jr $ra; nop` ends at 0x0011F70C) and
 * func_0011F718 at 0x0011F718; the `nop` at 0x0011F714 is the gap's other half
 * and belongs to no row. Nothing in the boot jumps to 0x0011F710.
 *
 * tools/check.py func_0011F710: the smallest C function is `jr $ra; nop`, so the
 * candidate is 8 bytes against the row's 4 - "boot: 8 bytes, expected 4", 0 of 1
 * places match.
 *
 * Same class as func_00127218 and func_00127340: a split between two entries,
 * fixed in tools/levels.py, not a C body. */
