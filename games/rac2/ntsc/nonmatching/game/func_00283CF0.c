/* func_00283CF0 - 16 bytes, 28 places (the boot and every level), retail at
   0x00283cf0: cvt.w.s $f12,$f12 ; mfc1 $v0,$f12 ; jr $ra ; nop.  Only the
   destination register differs: this cc1 always converts into a fresh FPR
   (`cvt.w.s $f0,$f12 ; mfc1 $v0,$f0`).  Nine spellings were measured (the
   cast direct, through a float local, through an int local, through an
   unsigned cast, with the argument used twice, with the argument used in an
   addition): all give $f0.  The SDK compiler in build/compiler/sdk gives
   the same words.  What would unblock it: a source that lets the
   conversion reuse the argument's register. 28 October 2026 current-master
   check of the retained direct cast: 16 bytes, 0/28; only the first word
   differs (`cvt.w.s $f0,$f12` versus `$f12`).

   31 October 2026 (Opus 5.5): 16/16 bytes, 0/28, two of four instructions differ
   (`cvt.w.s` and `mfc1`).  fix_truncsfsi2 alternative 0 converts into a SCRATCH:SF;
   the lreg dump already has the source reduced to `(set v0 (fix:SI $f12))`, and that
   scratch gets $f0.  In-place needs alternative 1, an SI value in an FPR.  Tried for
   that: `u.i = (int)a0` through a union, `u.f = a0; u.i = (int)u.f`, and
   `*(int *)&a0 = (int)a0`: all the same bytes (the last read in the lreg dump).
   Catalogue identity (339674bbbe3b46d9) with RAC1's handwritten func_001FA898, but
   at exactly 16 bytes that is below the AGENTS.md identity bar, so reported FAILED. */
typedef float f32;

int func_00283CF0(f32 a0) { return (int)a0; }
