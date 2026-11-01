/* func_L00_00400F30 - NOT MATCHED, and not a function: 4 bytes expected, 26 places.
 * last check.py line: "func_L00_00400F30: 0 of 26 places match (src/levels/shared/L00_003FFA70.c)"
 * with "00_aranos_tutorial: 8 bytes, expected 4" - the plain C is the retail's word
 * plus the `jr $ra` the retail does not have.
 *
 * The 4 bytes are one word, `daddu $v0, $a1, $zero` (0x00A0102D), and nothing else.
 * At every one of the 26 places the neighbourhood is the same:
 *
 *   jr $ra ; addiu $sp,$sp,0x1A0 ; nop ; <the word> ; nop ; <the next body>
 *
 * The word sits between two linker's nops, after the previous body's epilogue
 * (`jr $ra` and its delay slot) and before the next body, and nothing reaches
 * it: no branch, no `jal` and no pointer in any of the 26 programs has
 * 0x00400F30 (0x004039D0 in level 13) as its target - the level programs call
 * 0x00400F38 and 0x00403A00 there, never the word's own address.  The same
 * word, in the same place, in all 26 levels.
 *
 * So this is the residue the catalogue split off, not an entry point: the
 * defect class of func_L00_002DA390 (see nonmatching/levels/func_L00_002DA390.c)
 * and func_L00_002EC230, a tools/levels.py extent fix.  Plain C cannot
 * reproduce it either: this cc1 always emits `jr $ra` after a function's
 * body (8 bytes here), and the retail's body has no return at all.  Data
 * for the maintainers, nothing to attempt here. */
