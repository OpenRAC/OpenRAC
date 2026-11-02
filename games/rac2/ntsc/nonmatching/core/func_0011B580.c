/* Not a function: the row's eight bytes are

 *     0x0011B580  and    $a0, $a0, $v0
 *     0x0011B584  0xCDCDCDCD

 * The second word is the linker's fill between two objects - the same
 * 0xCDCDCDCD the boot's core.text carries between its SDK objects - so no C
 * can produce the row: a compiled function is never padded with it.  The first
 * word reads $v0, which nothing in the row sets.
 *
 * The row sits between func_0011B500 (which ends at 0x0011B57C) and
 * func_0011B588 (a real 104-byte body whose prologue starts at 0x0011B588).
 * symbol_addrs/usa-v2/symbol_addrs.txt already calls this address "leaf
 * boundary splat no longer finds (INCLUDE_ASM_FRAGMENT debris, 2 words)".
 *
 * tools/check.py with `int func_0011B580(int a0, int a1) { return a0 & a1; }`
 * gives 12 bytes against the row's 8 and differs at +0x0.  A tools/levels.py
 * split removes the row; there is no C body to find. */
