/* func_002828F0 - 16 bytes, 1 place (the boot), retail at 0x002828f0:
   lwc1 $f0,-0x76DC($gp) ; nop ; jr $ra ; mul.s $f0,$f0,$f12.  Same wall as
   func_002828B8: the gp relative float constant and the multiplication are
   right, but this cc1 moves the `jr $ra` into the delay slot of the
   `lwc1` where retail leaves a nop, so the candidate is 12 bytes.  The
   body needs the -G8 profile, so it belongs in 0027F128-g8.c.

   A 29 October isolated `-O2 -G8 -ffunction-sections -g` unit remains 12/16
   bytes and 0/1. Debug labels do not add the retail nop; the GP load still
   uses `0($gp)` rather than `-0x76DC($gp)`. Isolated
   `-fno-schedule-insns2` also remains 12/16 and 0/1, retaining the same
   `lwc1; jr; mul.s` sequence. */
typedef float f32;

extern float D_001A7914 __attribute__((sda));

f32 func_002828F0(f32 a0) { return D_001A7914 * a0; }
