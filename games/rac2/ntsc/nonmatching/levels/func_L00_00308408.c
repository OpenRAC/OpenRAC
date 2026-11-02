/* func_L00_00308408 - NON MATCHEE (wave 8, agent w8-09).
 *
 * Not attempted as a C body: the retail body needs ten saved registers and a
 * 128-bit copy, and this compiler cannot reach retail's nine-register frame
 * (it spills $fp as well).  What the retail bytes say, for the next attempt:
 *
 *  - frame 0x70, $s0-$s7, $fp and $ra saved - ten slots, the widest in this
 *    level's shared code;
 *  - the arguments are a base pointer, a count, a stride and another pointer;
 *    the guard is `addu $v1,$a1,$a3 ; slt $v0,$a1,$v1` (an empty range) and
 *    `slt $v1,$s1,$a0` (the end of the walk);
 *  - the walk steps by 32 bytes (`sra $a0,$s1,5 ; sll $s5,$v0,4`), reads a
 *    byte at the walked address and keeps a bit mask of the touched bytes in
 *    $s0 (`sllv $s0,$s7,$v1 ; and $v0,$v0,$s0`), skipping the bytes already
 *    set with `nor $v0,$zero,$s0 ; and $v1,$v1,$v0 ; sb $v1,0x0($s2)`;
 *  - func_308700 is called for every 32-byte group that still has a byte to
 *    set;
 *  - a 128-bit copy (`lq`/`sq`, the R5900 0x1E/0x1F opcodes) moves the block
 *    at 0x10($sp) into the block the group names, twice in the body (the words
 *    at +0x8C and +0xD4 of the retail).
 *
 * The blocker is the register count: with ten callee-saved registers this
 * compiler needs a ninth saved slot and spills $fp, which retail does not do.
 */

#include "include_asm.h"

typedef int TI128 __attribute__((mode(TI)));
