/* GPT-6.1 Sol high, v0786, USA v1.01, current pinned compiler.
 * -O2 -G0 -ffunction-sections -fno-strength-reduce (default profile ties, below):
 * 356/356 bytes, 0/1 places; only seven remainder scratch-register instructions
 * differ, starting at +0xCC. The 0x60 frame, fp-held helper result, both loop
 * schedules, pointer arithmetic, copy and epilogue now agree with retail.
 * Keeping the two unsuccessful-candidate increments on their own paths fixes
 * the helper spill and iteration scheduling in the older equal-size draft.
 * regalloc: the remainder's -1 is loop-hoisted into pseudo 119 (3 references,
 * 42 live instructions, one call), then spilled/rematerialized into a2.
 * Retail materializes it directly into v0 and reuses v0 for the sign predicate;
 * ours keeps the predicate in v1 and rounded dividend in v0.
 * Explicit quotient/subtraction and sign-rounded formulations do not resolve
 * allocation. Reopen on evidence for the constant's lifetime or hoisting.
 *
 * Opus 5.5 (opus-1, v1416), 2026-10-29, same compiler: the default profile
 * (-O2 -G0 -ffunction-sections, no -fno-strength-reduce) gives the same 356
 * bytes and the same seven differing words, so a separate unit is not needed.
 * Cause, from the dumps: jump1 turns the `% 8` sign fix into a conditional
 * move (GE 0 is canonicalized to GT -1, so mips gen_conditional_move forces
 * the -1 into a pseudo); loop.c then moves that constant out of the loop
 * ("move-insn savings 1"), it gets no callee-saved register (all nine are in
 * use) and reload rematerializes it in a2. Retail's -1 is a local pseudo in
 * v0, i.e. loop.c did not touch this loop, yet retail keeps the loop's
 * alignment nop, so the loop notes were there. A goto-built loop (no loop
 * notes) gives retail's exact remainder sequence but loses the nop and the
 * branch shapes; matched func_00303BF8 shows retail does hoist this -1 in
 * an ordinary loop. func_L06_003F3800 (nonmatching) has the same signature.
 * Not fixed by -frac2-keep-block-order or -frac2-gcse-unsplit-edges, natural
 * for/break, for/continue, nested-if while, or repeating the `% 8`
 * expression. Next: a source shape that leaves loop notes but makes loop.c
 * skip the loop (phony or multiple-entry), shared with func_L06_003F3800.
 */
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));

extern u32 D_L16_001FE7C0[];

extern s32 func_L16_00417788(void *);

extern s32 func_002ACBC8(void *, s32);

extern s32 func_002A77E0(s32);

extern float func_00282E80(void *, void *);

extern void func_L16_00410CC0(u8 *, u8 *);

/* Picks the row of the level's table that is closest to the handle and copies it
   into the handle's second half. */
void func_L16_00410E40(u8 *arg0)
{
    u32 *base;
    u32 *s2;
    u8 *s3;
    s32 fpi;
    u8 *p;
    s32 i;
    s32 best;
    s32 found;
    s32 b2;
    s32 rem;
    s32 result;
    f32 bestf;
    f32 f;

    base = D_L16_001FE7C0;
    i = 0;
    best = -1;
    s2 = *(u32 **)(arg0 + 0x68);
    bestf = 1024.0f;
    s3 = *(u8 **)(base + s2[92]);
    fpi = func_L16_00417788((void *)s2[147]);
    p = s3 + 0x10;
    do {
        f = func_00282E80(arg0 + 0x10, p);
        if (f < bestf) {
            bestf = f;
            best = i;
        }
        i++;
        p += 0x10;
    } while (i < 8);
    found = -1;
    b2 = func_002A77E0(8);
    i = 0;
    s2 = (u32 *)((u8 *)s2 + 0x200);
    while (i < 8) {
        rem = (i + b2) % 8;
        result = func_002ACBC8(s3 + (rem * 16 + 0x10), fpi);
        if (result == 0) {
            i++;
            continue;
        }
        if (rem != best) {
            found = rem;
            break;
        }
        i++;
    }
    qcopy(s2, s3 + (found * 16 + 0x10));
    func_L16_00410CC0(arg0, (u8 *)s2);
}
