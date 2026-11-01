/* near-miss: func_L20_00442E68 (20_yeedil, 200 bytes).
   The body and the loop condition are retail's, byte for byte; only the entry block
   differs. This cc1 and retail agree on the whole of 0x00442EA0..0x00442F2C (the
   "walk" body, the condition block and the epilogue), the frame is 0x20 and the size is
   retail's 200 bytes. The entry block differs in five instructions:
     retail: lui $v0,%hi(D_1FBB20) ; ... ; addiu $a0,$v0,%lo ; lw $v1,0x218($s1) ;
             sll $v1,$v1,2 ; addu $v1,$v1,$a0 ; lw $s0,0x0($v1) ; b COND ;
             daddu $s2,$v0,$zero
     this cc1: lui $v1,%hi(D_1FBB20) ; daddu $s2,$v1,$zero (early) ; addiu $v1,$v1,%lo
             (in place) ; lw $a0,0x218($s1) ; sll $v0,$a0,2 ; addu $v0,$v0,$v1 ;
             b COND ; lw $s0,0x0($v0)
   i.e. the table's %hi lands in $v1 instead of $v0, the table's address is materialised
   in place (so the %hi dies and its copy into $s2 has to happen early, which also swaps
   the callee-saved save order) and the index lands in $a0 instead of $v1. The whole
   difference is one decision: retail materialises the table's address into a register
   that is NOT the %hi register ($a0), so the %hi survives until the branch's delay slot
   and the copy into $s2 lands there; this cc1 reuses the %hi register, so the copy is
   scheduled early.
   Measured and rejected: the base as a variable before the loop (base in $s3, 216 bytes);
   the base loaded inside the loop (the loop rotates and the checks are duplicated, 252
   bytes); the base in a `char **tbl` variable used in both places (the whole 64-bit
   address is kept in $s2 and the COND's `addiu $v1,$s2,%lo` shape is lost, 192 bytes);
   the base through a `volatile` pointer (no change); the table typed `unsigned short *[]`
   (no change); the index as `unsigned int` (20 differing instead of 18); the subscript
   spelled as a pointer addition, `(char *)D_L20_001FBB20 + (i << 2)` (no change);
   the entry's index read as a separate statement (the COND's reload is then threaded away
   and the entry's branch skips it).
   Per-file flags measured on this unit: -fno-schedule-insns2 gives retail's size (200
   bytes) and one instruction less wrong, but it breaks the five functions already matched
   in L20_0043ECB0-g8.c (00442140, 00442740, 00444C18, 0043F510, 00440150 all stop
   matching), so the flag cannot be used for that unit; -fno-schedule-insns gives 196
   bytes; -g and -mno-split-addresses change nothing.
   A file of its own with -fno-schedule-insns2 keeps retail's size (200 bytes) and gets
   the entry block one instruction closer, but 17 instructions still differ (the whole
   entry block: the %hi's register, the address's destination register and the copy into
   $s2). No flag and no source shape tried reproduces retail's entry.
   The body needs the -G8 unit: it reaches D_L20_001B2FB8 through $gp. */
   Fourth wave (20_yeedil, 2026-10-20, worker 01): the entry-block wall is now
   narrower.  With `-G8 -mastra-keep-frame-order` (a per-file flag of the game's
   compiler, patch 0021) the prologue saves come out in retail's order - $s0, $s2,
   $ra, then the delayed $s1 - and the body stays retail's byte for byte: 200 bytes,
   the first difference moves to +0x0c.  What is left is ONE decision, the one the
   third wave named: retail materialises the table's address into a register that is
   NOT the %hi register ($a0, so the %hi in $v0 survives into the branch's delay slot
   where the copy into $s2 lands), while this cc1 reuses the %hi register ($v1, so
   the copy into $s2 is scheduled early and the %hi lands in $v1).  Measured here and
   rejected: the table as `void *[]` instead of `char *[]`, the subscript as a
   pointer addition `*(unsigned short **)((char *)D_... + (idx << 2))` and as
   `(idx * 4)`, the base in a `char **` variable, the index as `unsigned int`, the
   state loaded before or after the table lookup - all give the same two registers.
   The same flag moves func_L20_00447640's entry block the same way (264 bytes, the
   difference at +0x04); there the retail wants the %hi in $v1 and the address in
   $a0, this cc1 puts both in $v0 and sign-extends the index into $a1.
   The flag is per file: it needs its own -G8 unit, it cannot go on
   L20_0043ECB0-g8.c (whose five matched functions it would change).

#include "common.h"

extern char *D_L20_001FBB20[] __attribute__((nosda));

extern char *D_L20_001B2F9C __attribute__((nosda));

extern int D_L20_001B2FB8 __attribute__((sda));

extern void func_003BA2F8(char *);

void func_L20_00442E68(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);
    unsigned short *p = (unsigned short *)D_L20_001FBB20[*(int *)(state + 0x218)];
    int i;

    goto cond;
loop:
    {
        char *e = D_L20_001B2F9C + ((*p & 0x7FFF) << 8);

        if (e == 0) {
            goto out;
        }
        *(unsigned short *)(e + 0x36) = 0x7F80;
        func_003BA2F8(e);
    }
    p++;
cond:
    i = *(int *)(state + 0x218);
    if ((i & 0xFF) == 0xFF) {
        goto out;
    }
    if (D_L20_001B2FB8 < i) {
        goto out;
    }
    if (p == 0) {
        goto out;
    }
    if (*(short *)(p - 1) >= 0 || p == (unsigned short *)D_L20_001FBB20[i]) {
        goto loop;
    }
    goto out;
out:
    ;
}

/* ---- wave 4 (20_yeedil, 2026-10-20, worker 01): the entry block, re-measured ----
 *
 * The attempt is in src/levels/20_yeedil/L20_00442E68-g8.c (a unit of its own: the
 * body reaches D_L20_001B2FB8 through $gp) and it is 200 bytes, retail's size.  The
 * body - the walk at 0x00442EA0, the condition block and the epilogue - matches byte
 * for byte; the entry block is 10 words off.
 *
 * What the fourth wave measured (all in a file of its own, so the flags are per file):
 *
 *  - `-fno-schedule-insns2` is the best flag: it keeps retail's size (200 bytes) and
 *    puts the table's absolute address in `$a0` (retail's register) and the split copy
 *    `daddu $s2,$v0,$zero` in the branch's delay slot (retail's place) - the two things
 *    the third wave called the wall.  What is left is the `%hi`'s register ($v1 here,
 *    $v0 in retail), the prologue's save order ($s0, $s1, $s2, $ra here; $s0, $s2, $ra,
 *    $s1 in retail) and the position of the `lui` (after the saves here, before them
 *    in retail).  With the default flags the entry block is 18 words off and the
 *    `%hi` is coalesced with the absolute address (`addiu $v1,$v1,%lo`), so the split
 *    copy has to be scheduled early.
 *  - `-fno-schedule-insns` gives 196 bytes; `-mastra-keep-frame-order` changes nothing
 *    here (the retail's save order is not the ascending one, so the retail's compiler
 *    reordered the saves - the scheduler's own order, not the emission order).
 *  - The "second C name" recipe of the wave-3 note (two variables for the table's
 *    address) was measured again: with the table in a `char **` variable used in both
 *    places the whole 64-bit address is kept in `$s2` and the condition block's
 *    `addiu $v1,$s2,%lo` shape is lost (192 bytes); with the second name in a nested
 *    block the compiler folds the variable away and the result is the baseline.
 *  - The state read after the table lookup (the state is then read twice) puts the
 *    `lui` before the prologue's saves, as in retail, but the `%hi` lands in `$v1` and
 *    the absolute address in `$v1` as well.
 *
 * The residue is one register-allocation decision: retail materialises the table's
 * address into `$a0`, a register that is NOT the `%hi`'s, so the `%hi` survives until
 * the branch's delay slot; this cc1 reuses the `%hi`'s register, so the copy into
 * `$s2` has to be scheduled early.  No source shape and no per-file flag tried
 * separates the two.
 */

/* ---- wave 5 (20_yeedil, 2026-10-21, worker 02): the split address, measured -----
 *
 * The split address of D_L20_001FBB20 DOES come out with the corrected recipe of the
 * common note ("a second assignment of the same symbol with a use after it, and the
 * global used more than once"): with
 *
 *     char *tbl = D_L20_001FBB20;      /* the first use, before the loop *\/
 *     s0 = tbl[*(int *)(state + 0x218)];
 *     for (;;) {
 *         ... the body ...
 *         tbl = D_L20_001FBB20;         /* the re-assignment, after the nested block *\/
 *         if (s0 == tbl[idx]) continue;  /* the second use *\/
 *     }
 *
 * the compiler emits retail's `daddu $sN, $vN, $zero` (the copy of the %hi) and
 * `addiu $vN, $sN, %lo(D_L20_001FBB20)` at the use - with the DEFAULT flags, which
 * coalesce the %hi with the absolute address and lose the shape.  Without the
 * re-assignment the compiler materialises the whole address in a register instead.
 *
 * With the default flags this spelling is 212 bytes against retail's 200: the register
 * pressure is higher than retail's (the loop-invariant motion hoists the two constants
 * 0xFF and 0x7F80 out of the loop into callee-saved registers where retail
 * rematerialises them inside the loop), so five registers are saved instead of three.
 * The `-fno-schedule-insns2` measurement of the fourth wave (200 bytes, ten words of the
 * entry block still off) is the better one; this note only adds the re-assignment
 * recipe for the split address, which the fourth wave did not record for this
 * function. */
