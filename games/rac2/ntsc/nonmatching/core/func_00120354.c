/* from src/core/0011FA70.c: the row starts four bytes early, in the linker's
 * fill, and the body behind it is plain C.
 *
 *   0x120354  0xCDCDCDCD          the linker's fill word between two objects
 *                                 (the pattern the other remnant rows record);
 *   0x120358  addiu $sp,$sp,-0x10  the real entry: a 0x10 frame;
 *   0x12035C  sd $ra,0x0($sp);
 *   0x120360  jal func_00115E28   the callee never returns - its own 64 bytes
 *                                 at 0x115E28 end in `jal func_00115E28` and
 *                                 hold no jr $ra - so the frame gets no
 *                                 epilogue;
 *   0x120364  nop                 the call's delay slot.
 *
 * The C below reproduces those 16 bytes exactly. Measured with the SDK
 * compiler (config/cflags.txt gives src/core/0011FA70.c the
 * `sdk -O2 -G2 -ffunction-sections` profile):
 *
 *   tools/check.py func_00120354 --diff
 *     boot: 16 bytes, expected 20
 *       00120354  addiu       $sp, $sp, -0x10              | pref        0x0D, -0x3233($t6)
 *       00120358  sd          $ra, 0x0($sp)                | addiu       $sp, $sp, -0x10
 *       0012035C  jal         func_000000                  | sd          $ra, 0x0($sp)
 *       00120360  nop                                      | jal         func_115E28
 *       00120364                                           | nop
 *
 * Word for word one place behind retail: the candidate is retail's body from
 * 0x120358 on. Placed where retail has it, the same object derives clean
 * (Placement.derive at 0x120358: OK, func_00115E28 = 0x115E28); placed at the
 * row's own address 0x120354 it fails on the fill word alone ("instruction at
 * +0x0 differs"), and 0xCDCDCDCD is an instruction no compiler emits.
 *
 * So the row is what is wrong, not the body: a tools/levels.py split that
 * starts the row at 0x120358 (16 bytes) and gives 0x120354 to a filler row
 * unblocks it, the same shape as the recorded func_001158F4 and func_00127218
 * rows. The `noreturn` is what gives retail's shape: without it the compiler
 * keeps the epilogue and the body is 32 bytes; with the call alone it is a
 * tail call of 4 bytes. The second call is what keeps the frame - it is dead
 * code after a noreturn call, so the compiler drops it and the epilogue with
 * it - so any statement after the call does the same.
 */
#include "include_asm.h"

extern void func_00115E28(void) __attribute__((noreturn));

extern void func_00120368(void);

void func_00120354(void) {
    func_00115E28();
    func_00120368();
}
