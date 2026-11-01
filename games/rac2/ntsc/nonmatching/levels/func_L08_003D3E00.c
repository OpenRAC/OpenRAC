/* 0 of 1 place match - not attempted in C: the row carries a 128-bit transfer word.
   At 0x003D3E5C, in the middle of the mode dispatch, the retail word is 0x7FA00000 -
   opcode 0x1F (SQ), rs = $sp, rt = $zero, offset 0 - i.e. `sq $zero, 0x0($sp)`, a 16-byte
   zero written to a stack local, which rabbitizer prints as INVALID.
   It is the MMI/128-bit class the brief sends straight to a near miss, and the sixth wave's
   note applies: the TI128 `mode(TI)` typedef does reach lq/sq, but this cc1 folds a constant
   destination offset into the store where retail materialises the pointer first - a wall
   reconfirmed three times by the seventh wave, so no spelling was spent here.
   What the rest of the row shows: switch on the byte at object+0x20, case 0 sets 0x20(p)=1,
   case 1 tests object[0xBC] and either promotes to mode 2 or falls through, mode 2 runs
   func_002B0A68(object, 3.0f, 6.0f, &stack, 0x7F, 0xFF, 0.15f, 3.0f, 6.0f) and then
   func_0029F338(object).
   What unblocks it: the same TI128 destination lever as func_L00_002D5098. */

#include "include_asm.h"

INCLUDE_ASM("asm/levels", func_L08_003D3E00);

/* Wave 24 (worker 05): attempted, and the wall is NOT the TI128 one - it is the
   block layout of the last block.  The C below reproduces every branch, every
   delay slot and the block order (entry, .L38, .L48, .L5C, exit):

     lbu  $v1, 0x20($s0)
     beq  $v1, $a0, .L48        ; the state-1 case is out of line
     slti $v0, $v1, 0x2
     beql $v0, $zero, .L38      ; the state>=2 case is out of line
     addiu $v0, $zero, 0x2
     beql $v1, $zero, .L90      ; the state-0 case stores 1 and exits
     sb   $a0, 0x20($s0)
     b    .L90
     ld   $s0, 0x10($sp)
   .L38:
     beq  $v1, $v0, .L5C        ; the state-2 case is out of line too
     nop
     b    .L90
     ld   $s0, 0x10($sp)
   .L48:
     lbu  $v0, 0xBC($s0)
     beqz $v0, .L90
     addiu $v0, $zero, 0x2
     b    .L90
     sb   $v0, 0x20($s0)
   .L5C:
     sq   $zero, 0x0($sp)
     lui  $at, 0x4040
     mtc1 $at, $f12
     daddu $a0, $s0, $zero
     ...
     jal  f_002B0A68
     swc1 $f0, 0x8($sp)
     jal  f_0029F338
     daddu $a0, $s0, $zero

   but the last block is one instruction short (176 bytes against 180) and
   the two differences are the same thing: the scheduler puts the 16-byte zero
   store late (10th in the block against retail's 1st) and fills the .L38
   branch's delay slot with the call's `daddu $a0,$s0,$zero` where retail has
   a nop.  check.py (08_tabora): "176 bytes, expected 180".

   What is measured and right:
   - the state byte at 0x20 is read as an unsigned byte (retail `lbu`) but
     compared as a signed int (retail `slti $v0,$v1,0x2`), so the value has
     to be an int holding a zero-extended byte.
   - the "state == 1" case has to be the else of a "!= 1" test: written as
     `if (v1 == 1) {...} else {...}` the compiler inverts the branch and emits
     `bne` with the case inline, retail branches to it with `beq`.
   - the big block has to be reached by a `goto` (a branch out of line),
     otherwise the compiler keeps it as the fall-through of .L38 and the
     branch polarity of the "state == 2" test flips (`bnel` against retail's
     `beq`).
   - the 16-byte local is a mode(TI) object (retail `sq $zero,0x0($sp)`) with
     a float written at its offset 8 (`swc1 $f0,0x8($sp)`); a union of the
     two is what the compiler accepts.  */
extern void func_002B0A68(void *, int, void *, int, float, float, float, float);
extern void func_0029F338(void *);

typedef union {
    ti_word v;
    float f[4];
} Quad;

void func_L08_003D3E00_w24(unsigned char *a0) {
    unsigned char *s0 = a0;
    int v1 = s0[0x20];
    Quad local;

    if (v1 != 1) {
        if (v1 < 2) {
            if (v1 == 0) {
                s0[0x20] = 1;
            }
        } else if (v1 == 2) {
            goto L5C;
        }
    } else {
        if (s0[0xBC] == 0) {
            return;
        }
        s0[0x20] = 2;
        return;
    }
    return;
L5C:
    local.v = 0;
    local.f[2] = 0.15f;
    func_002B0A68(s0, 0x7F, &local, 0xFF, 3.0f, 6.0f, 3.0f, 6.0f);
    func_0029F338(s0);
}
