/* func_L00_004003F0 - 444 bytes, 27 places (the 444-byte family: func_00326A10, func_0032F3D0,
   func_L00_003DDEC8 are byte-for-byte the same shape).  Tier fable, 28 October 2026.

   Best check.py: 436/444 bytes, 0/27 places, as its own unit
   src/levels/shared/L00_004003F0-g8.c with the cflags line
   `src/levels/shared/L00_004003F0-g8.c  -O2 -G8 -fno-expensive-optimizations -fno-schedule-insns -ffunction-sections`
   (take the marker out of L00_003FFA70.c; D_L00_001B0CA0 is a $gp read, so -G8).  The same C
   with the default schedulers (-O2 -G8 -fno-expensive-optimizations -ffunction-sections) is also
   436/444: it gets retail's `daddu $a0,$v0,$zero` in the load slot of the first two chains, but
   y lands in $v0 and the FPR roles come out -0.5->f23, 1.0->f22, 0.0->f20, 0.5->f21 (the opus-2
   analysis of the roles below holds only with sched1 off).  The body is the opus-2 draft,
   unchanged; the reload and scheduler reading below is new and measured on this cc1's dumps
   (tools/regalloc.py, cc1 -dl -dg -dS -dR, and the archive sources).

   The two remaining differences are both compiler-side; no C shape reaches them:

   1. The ghost $s0 (frame 0xA0, three GPR saves, 0x8710 in $s1).  It is the GPR scratch of
      fix_truncsfsi2.  mips.md's pattern is `(set d (fix:SI f)) (clobber (match_scratch:SI "=d"))
      (clobber (match_scratch:SF "=f"))`, and the R5900 branch of its template prints only
      `cvt.w.s %3,%1` + `mfc1 %0,%3`: the GPR scratch is allocated by reload and never printed.
      Evidence: (a) in the greg dump every `cvt.w.s` here needs two spill registers
      ("Spilling for insn 50: reg 4, reg 33"; 98: 4, 33; 165: 2, 33), i.e. $a0/$a0/$v0 and $f1;
      (b) finish_spills sets regs_ever_live for every register in used_spill_regs whether or not
      an insn ends up using it, so a scratch in $s0 is saved and never used; (c) find_reload_regs
      calls maybe_mark_pseudo_spilled, so a pseudo living in the chosen register is evicted and
      retry_global_alloc puts it in the next free callee-saved register: 0x8710 from $s0 to $s1,
      exactly retail; the sibling func_L02_003FF6E8 (ctx rematerialised from its frame-address
      REG_EQUIV once evicted from $s2, state in $s3) is the same story; (d) 9 of the 10 ghost-save
      functions named in that sibling's notes contain `cvt.w.s` (func_00326A10, func_0032F3D0,
      func_L00_003DDEC8, func_002D2188, func_L02_003C43F0, func_L02_003FE8B8, func_L04_00416E38,
      func_L05_00421B98, func_L02_003FF6E8; func_0034A4E0 does not and needs another look), and in
      each the ghost is a callee-saved register next to the ones the body uses; (e) retail's
      `daddu $a0,$v0,$zero` sits above the first two chains, which sched2 can only do when the
      chain's clobbered scratch is not $a0; here it is $a0, so the draft keeps the move in the jal
      slot.  Why this cc1 cannot pick $s0: order_regs_for_reload puts registers with no live
      pseudo and not live at the insn first, in REG_ALLOC_ORDER (the identity on MIPS: v0, v1, a0,
      ..., t7, s0, ...), so at a chain where only $v0 (id) and $v1 (y) are live it always takes
      $a0.  Retail's reload took the register of a live callee-saved pseudo (0x8710's $s0 here,
      ctx's $s2 in the sibling) ahead of twelve free call-clobbered ones; the matched
      func_0033C028 (two chains, no live callee-saved pseudo at them, scratches $v0) shows the
      choice is conditional, not a fixed preference for s-registers.  Next, compiler-side:
      compare the archive's order_regs_for_reload / new_spill_reg with what the 21 ghost functions
      need (a rule that ranks a register holding a live pseudo above unused call-clobbered ones
      when ... ?), test it on the 21 and on every matched function with `cvt.w.s`.  Not a source
      shape: the text pointer idea (opus-1/2, sibling) cannot give $s0 to the ghost and $s1 to
      0x8710 in local-alloc (measured there), and is not needed once the scratch explains it.

   2. The float chains.  Retail issues each `(int)(x*k+0.5f)` chain contiguously (lwc1; daddu
      a0,v0; mul.s; add.s; cvt.w.s; mfc1; mtc1; nop; cvt.s.w) and only then the argument
      constants (f22, f23, lui a1, f21, ori a1, f12, mov.s x3).  The archive's R5900 model (fmac
      ready cost 3, loads 2, two ALUs, no transformer touches it) leaves gaps that this cc1's
      sched2 fills with the ready constant loads, with or without sched1 (sched1 additionally
      hoists them above the first call and above the chain).  With sched1 on, this cc1 also hoists
      `a0 = v0` above the fix insn, which frees $v0 for y (retail: $v1), so retail's pre-reload
      order had the copy after the chain; its `daddu` in the load slot is sched2's.  A `do { }
      while (0)` around the rounding (diagnostic only; not landable) stops the interleave inside
      the chain but not the hoisting of the constants above it.  Unblocked only together with 1
      (the scratch register decides whether sched2 may hoist the move) and a reason the constant
      loads were not ready until after the chain in retail.

   What the draft reproduces (opus-2, each measured): returns 1 on both paths; 454.0f
   rematerialised at both func_00300A00 calls (with -fno-expensive-optimizations a literal float
   argument goes straight into its register; the shared -0.5/1.0/0.0 must be variables); five
   FPR saves; FPR roles 0.5->f20, 0.0->f21, -0.5->f22, 1.0->f23, 380->f24 and y in $v1 with
   -fno-schedule-insns (local-alloc order floor_log2(refs)*refs/(death-birth), ties by birth:
   `w = 380.0f` first, ox/sc between the multiply-add and the truncation, `z = 0.0f` in the
   argument list); the second half (func_002ED578, the sprintf, func_00283CF0, the draw call) is
   retail's word for word apart from the chain order.

   Declarations needed are below (D_001A904C is boot data read with lui/lw, D_L00_001B2704 and
   D_L00_001B2C10 with lui/lwc1 and lui/lw, D_L00_001B0CB8 with lui/addiu, D_L00_001B0CA0 via $gp). */

#include "common.h"
extern int D_L00_001B0CA0 __attribute__((sda));
extern int D_001A904C[] __attribute__((sda));
extern float D_L00_001B2704[] __attribute__((sda));
extern int D_L00_001B2C10[] __attribute__((sda));
extern char D_L00_001B0CB8[] __attribute__((nosda));
extern int func_00336D50(int, int);
extern void func_00300A00(int, int, float, float, float, float, float);
extern void func_002ED578(int, int *, int *, int *);
extern int func_00115DA8(char *, char *, ...);
extern int func_00283CF0(float);
extern void func_0027F5B8(int, int, unsigned long, char *, int);

int func_L00_004003F0(void)
{
    char text[64];
    int a, b, c;
    int id, y;
    float ox, sc, z, w, t;

    if (D_L00_001B0CA0) {
        w = 380.0f;
        id = func_00336D50(D_001A904C[0] + 0x8710, 0xC1);
        t = D_L00_001B2704[0] * w + 0.5f;
        ox = -0.5f;
        sc = 1.0f;
        y = (int)t;
        func_00300A00(id, 0x60442D00, 454.0f, (float)y, ox, sc, z = 0.0f);
        id = func_00336D50(D_001A904C[0] + 0x8710, 0xC2);
        y = (int)(D_L00_001B2704[0] * w + 0.5f);
        func_00300A00(id, 0x55F0C070, 454.0f, (float)y, ox, sc, z);
        func_002ED578(D_L00_001B2C10[0], &a, &b, &c);
        func_00115DA8(text, D_L00_001B0CB8, a, b, c);
        func_0027F5B8(func_00283CF0(405.0f), (int)(D_L00_001B2704[0] * 372.0f + 0.5f),
                      0x80808080UL, text, 128);
    }
    return 1;
}
