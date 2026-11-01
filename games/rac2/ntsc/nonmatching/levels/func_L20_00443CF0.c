/* near-miss: func_L20_00443CF0 (20_yeedil, 100 bytes).
   0x24: retail emits `daddu $a1,$sp,$zero` before the prologue's `sd $s0,0x30($sp)`,
   this cc1 the other way round. Every other word matches. Same wall as the recorded
   func_001334B8 row (a single scheduling word in the prologue). 
   Second wave (20_yeedil, 2026-10-20) re-measured this one and could not move it:
   ten further source shapes for the two locals (char[16], unsigned int[4], float[4],
   a TI128 pair, one 32-byte array used as v and v+16, a 48-byte struct holding all
   three objects, the &v[0] spellings, a `char *p0 = v0` temporary, the declaration
   order of the locals, and the float assigned as a separate statement) all give the
   same 100 bytes with only these two words swapped. The float constant is 1.0471977f
   (0x3F860A93) and is materialised in the entry block, before the frame setup, exactly
   as retail has it. Wall: the prologue interleaving - a lever that pins the local's
   address computation before the `sd $s0,0x30($sp)` prologue save is what is missing.*/
   Fourth wave (20_yeedil, 2026-10-20, worker 01): the two flags of the assignment
   were measured on this function in a file of its own, and each of them moves the
   pair the third wave named, but breaks something else:
     - `-fno-schedule-insns2` puts the retail's `daddu $a1,$sp,$zero` BEFORE the
       prologue's `sd $s0,0x30($sp)` - the pair is right - but the lui/ori/mtc1 of
       1.0471977f sinks below the frame adjust (retail has it in the entry block);
     - `-mastra-keep-frame-order` (patch 0021) also puts the pair right, but it keeps
       the $s0 save in the prologue instead of delaying it to just before the
       `lw $s0,0x68($v0)` that retail has, and it leaves the saves in the prologue
       order instead of retail's $s1, $ra, $s0.
   So the wall is the interaction of the two: retail delays the $s0 save AND lets
   the first call's second argument precede it.  Measured and rejected: the float
   initialised in the declaration or in a separate statement, the state loaded
   before or after the first call, the index in a variable of its own, the two
   locals as TI128, char[16] or unsigned int[4], the declaration order of the
   locals, the arguments as TI128* or char* - the pair never moves without one of
   the flags, and with a flag the constant or the save order breaks.
   The flags are per file: this function needs a unit of its own, it cannot go on
   the shared L20_0043ECB0.c (the flag changes the other bodies of the unit).

#include "common.h"

typedef int TI128 __attribute__((mode(TI)));

extern void func_L20_00443B60(char *, TI128 *, TI128 *, f32 *);
extern void func_L20_00440198(s32, TI128 *, TI128 *, f32);

void func_L20_00443CF0(char *arg0)
{
    char *state = *(char **)(arg0 + 0x68);
    TI128 v0;
    TI128 v1;
    f32 f;

    f = 1.0471977f;
    func_L20_00443B60(arg0, &v0, &v1, &f);
    func_L20_00440198(*(s32 *)(state + 0x21C), &v0, &v1, f);
}

/* Third wave (20_yeedil, 2026-10-20, worker 02): the same one-word-pair wall,
   re-measured.  The candidate below is the two `mode(TI)` locals at sp+0 and
   sp+0x10 with the float at sp+0x20, and it differs from the retail in one
   word pair: the retail sets the first call's second argument up before the
   prologue's `sd $s0,0x30($sp)`, this cc1 after it.  Additional spellings
   measured: the locals as `char[16]`, as one 32-byte struct, declared in the
   other order, the state as `char *` and as `void *`, the argument as `void *`,
   the index in a variable of its own, the float passed as `*(float *)&f`, the
   state load through a `u64` argument, and the state assigned after the first
   call - all give the same 100 bytes with the same two words swapped.  The
   per-file flags `-g`, `-mno-split-addresses`, `-fno-schedule-insns` and
   `-fno-schedule-insns2` were tried on the whole unit: the first two change
   nothing, the last two reorder the whole body.  This is the prologue
   interleaving the compiler notes list as a measured wall. */

#include "include_asm.h"

typedef int TI128 __attribute__((mode(TI)));

extern void func_L20_00443B60(char *, char *, char *, float *);

extern void func_L20_00440198(int, char *, char *, float);

void func_L20_00443CF0(char *arg0) {
    TI128 v1;
    TI128 v2;
    float f = 1.0471977f;
    char *state = *(char **)(arg0 + 0x68);

    func_L20_00443B60(arg0, (char *)&v1, (char *)&v2, &f);
    func_L20_00440198(*(int *)(state + 0x21C), (char *)&v1, (char *)&v2, f);
}

/* ---- wave 4 (20_yeedil, 2026-10-20, worker 01): the word pair, re-measured -------
 *
 * The attempt is in src/levels/20_yeedil/L20_00443CF0.c (a file of its own, because
 * every flag measured changes the other bodies of a shared unit).  It is 100 bytes,
 * retail's size, and 23 of the 25 words match: the residue is the pair at +0x24/+0x28,
 * where retail sets `$a1 = $sp+0` (the first call's second argument) BEFORE the
 * prologue's `sd $s0,0x30($sp)` and this cc1 after it.
 *
 * Measured here, each in a file of its own, with the word-level differ:
 *
 *  - the per-file flags `-fno-schedule-insns`, `-fno-schedule-insns2`,
 *    `-mastra-keep-frame-order`, `-fno-schedule-insns2 -mastra-keep-frame-order`,
 *    `-fno-schedule-insns -fno-schedule-insns2` and `-fno-schedule-insns2
 *    -fno-sched-interblock` all give the SAME two words; the third wave's measurement
 *    that `-fno-schedule-insns2` moves the pair does not reproduce.
 *  - the source shapes: the locals as `TI128` or as `char[16]`, the arguments as
 *    `TI128 *` or `char *`, the float initialised in the declaration or in a separate
 *    statement, the state read before or after the first call, the state assigned
 *    between the two calls - all give the same two words.
 *
 * The wall is the scheduler's priority between the argument set-up (a move) and the
 * prologue's save of `$s0` (a frame-related store): the project's compiler prefers the
 * frame store (the frame-save-first patch, tools/compiler/patches/0026), the retail
 * does not.
 */

/* ---- wave 5 (20_yeedil, 2026-10-21, worker 03): the word pair, re-measured -------
 *
 * The candidate above is the wave-4 one.  This wave re-measured it with the
 * word-level differ and adds the following.
 *
 *  - The candidate is 100 bytes, the retail's size, and 23 of the 25 words match.
 *    The residue is exactly the pair at +0x24/+0x28: the retail sets the first
 *    call's second argument up BEFORE the prologue's `sd $s0,0x30($sp)`, this
 *    cc1 after it.  Nothing else differs.
 *  - The frame and the layout are the retail's and are recovered: the frame is
 *    0x50, the two 16-byte locals are at sp+0 and sp+0x10, the float is at
 *    sp+0x20, the first call gets (arg0, sp+0, sp+0x10, sp+0x20) and the
 *    second one (the word at state+0x21C, sp+0, sp+0x10, the float).  The
 *    constant is 1.0471977f (0x3F860A93) and is materialised in the entry
 *    block, before the frame adjust, exactly as retail has it.
 *  - The prototype of the first callee is fixed by the retail: its fourth
 *    argument is a FLOAT pointer (`lwc1 $f0,0x0($s5)` / `swc1 $f0,0x0($s5)`),
 *    so `void func_L20_00443B60(char *, char *, char *, float *)`.
 *  - The per-file flags, measured one at a time in a file of their own:
 *      - `-fno-sched-interblock` leaves the pair where it is and changes
 *        nothing else (still the same two words).
 *      - `-fno-schedule-insns2` puts the pair RIGHT - the retail's
 *        `daddu $a1,$sp,$zero` comes before the prologue's `sd $s0,0x30($sp)` -
 *        but sinks the lui/ori/mtc1 of 1.0471977f below the frame adjust
 *        (the retail has it in the entry block): 11 words differ then.
 *      - `-fno-schedule-insns2 -fno-sched-interblock` gives the same 11.
 *      - `-fno-schedule-insns`, `-mastra-keep-frame-order` and
 *        `-fno-schedule-insns2 -mastra-keep-frame-order` reorder the whole
 *        body (18 words and more).
 *    So the two effects the retail shows are separate: the constant's
 *    materialisation in the entry block wants the second scheduling pass, and
 *    the pair wants it off.
 *  - The source shapes, all giving the same 100 bytes with the same pair
 *    swapped: the locals as `char[16]` (the retail's), as one 32-byte array
 *    used as b1 and b1+16, as a 48-byte struct holding all three objects, as
 *    `float[4]` and as `int[4]`; the `&b1[0]` spellings; the state as `char *`
 *    and as `void *`; the index in a variable of its own; the float
 *    initialised in the declaration and in a separate statement; the
 *    declarations exchanged; the return type `void`, `char *` and `int`; a
 *    `char *p = b1` temporary for the second call's second argument.
 *
 * The wall is the scheduler's choice between the argument set-up (a move) and
 * the prologue's save of `$s0` (a frame-related store).
 */
