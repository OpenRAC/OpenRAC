/* Not a function: the row's twelve bytes are

 *     0x0011C8C8  sw     $a1, 0x0($a0)
 *     0x0011C8CC  nop
 *     0x0011C8D0  addiu  $v0, $v0, 0xD8

 * There is no jr $ra.  The last word adds 0xD8 to $v0, the return register,
 * which nothing in the row sets: `addiu $v0,$v0,imm` is the %lo half of a
 * global-address pair, and the `lui` that would give it its %hi is not in the
 * row.  The nop in the middle is not a delay slot either - the word before it
 * is a store, not a branch - and no compiler emits a free nop between two
 * instructions.
 *
 * The nearest C spellings, measured with the SDK compiler of this object
 * (src/core/0011C8B0.c is `sdk -O2 -G2 -ffunction-sections`):
 *
 *     int f(int *a0, int a1) { *a0 = a1; return (int)a0 + 0xD8; }
 *         sw $a1,0($a0) ; j $31 ; addu $v0,$a0,0xD8
 *     int f(int *a0, int a1) { *a0 = a1; return 0xD8; }
 *         sw $a1,0($a0) ; j $31 ; addiu $v0,$zero,0xD8
 *     int f(int *a0, int a1) { *a0 = a1; return *a0 + 0xD8; }
 *         sw $a1,0($a0) ; j $31 ; addu $v0,$a1,0xD8
 *
 * Word 0 of the first spelling is retail's word 0; the rest cannot be, because
 * the compiler always ends a function with a jump, and retail's second word is
 * a nop where the `j $31` belongs.  tools/check.py reports
 * "instruction at +0x4 differs" for it.
 *
 * The row sits between func_0011C8B0 (24 bytes, already matched: it ends with
 * `jr $ra` at 0x0011C8C0 and its delay slot at 0x0011C8C4) and func_0011C8D8
 * (640 bytes, whose prologue starts at 0x0011C8D8), so no part of it belongs
 * to a body.  A tools/levels.py split removes the row; there is no C body to
 * find. */
