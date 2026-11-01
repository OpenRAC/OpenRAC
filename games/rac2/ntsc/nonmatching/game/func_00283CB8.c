/* func_00283CB8 - 16 bytes, 28 places (the boot and every level), retail at
   0x00283cb8: cvt.w.s $f0,$f12 ; cvt.s.w $f0,$f0 ; jr $ra ;
   sub.s $f0,$f12,$f0.  Retail keeps the truncated integer in the FPU;
   this cc1 moves it through a GPR (`cvt.w.s $f0,$f12 ; mfc1 $v0,$f0 ;
   mtc1 $v0,$f0 ; nop ; cvt.s.w $f0,$f0`), so the candidate is 28 bytes
   instead of 16 in the current translation unit.  Eight spellings were measured (the cast through a local,
   through an int local, as an expression, with the cast applied twice, with
   the argument used twice): all give the same GPR round trip.  The SDK
   compiler in build/compiler/sdk gives the same words.
   A 29 October 2026 replay in the original translation unit confirms 28 bytes, 0/28; the return sequence begins four bytes later than retail. Current-master probes: `a0 - __builtin_rintf(a0)`, `nearbyintf`,
   `truncf`, and `roundf` each emit 60 bytes because this compiler lowers
   these builtins through a library call; they do not expose the conversion as
   inline FPU operations. Also, a C cast rounds toward zero,
   whereas `cvt.w.s` follows the current FPU rounding mode, so matching the
   instruction sequence must preserve the intended semantics rather than
   relying on the cast's apparent similarity.

   31 October 2026 (Opus 5.5): cc1's only `cvt.s.w` templates are
   `mtc1 %1,%0 ; cvt.s.w %0,%0` and `l.s %0,%1 ; cvt.s.w %0,%0` (floatsisf2; strings of
   build/compiler/tools/cc1 and build/compiler/sdk/cc1), so a `cvt.s.w` directly
   after `cvt.w.s` cannot come from this compiler.  Recompiled: 28/16 bytes, 0/28.
   `register int` gives the same bytes.  Reported as HANDWRITTEN; no RAC1 copy
   of this body exists. */
typedef float f32;

f32 func_00283CB8(f32 a0) { f32 b = (f32)(int)a0; return a0 - b; }
