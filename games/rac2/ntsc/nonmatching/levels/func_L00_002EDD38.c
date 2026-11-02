/* from src/levels/shared/L00_002E5918.c: retail, 16 bytes, 27 places:
   lwc1 $f0,-0x76D4($gp) ; nop ; jr $ra ; mul.s $f0,$f0,$f12.
   Twin of func_L00_002EDCB8, three floats later in .sdata (0x001A791C).
   The candidate is 12 bytes with the same load and the same `mul.s` operand
   order; the missing word is the `nop` between the load and the jump.
   Measured this wave (wave 8 w06): -O0, -O1, -O2, -O3, -G0, -G8,
   -fno-schedule-insns, -fno-delayed-branch, -ffast-math, a volatile global
   and a double read of the global all give the compact three-instruction form
   (or a different order); nothing reaches 16 bytes.  Assembling the retail's
   exact four instructions with the game's `as` gives 12 bytes too, so the
   `nop` is not the assembler's FPR-hazard nop: the compiler has to emit it.
   Marked G8. */
extern float D_001A791C __attribute__((sda));

float func_L00_002EDD38(float a0) {
    return D_001A791C * a0;
}
