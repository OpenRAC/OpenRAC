/* func_00282C90 - 8 bytes, 28 places (the boot and every level). Every
   USA v1.01 placement is `jr $ra; sqc2 $vf0,0($a0)`: the second instruction
   is the live return delay slot and a VU0/COP2 operation. The retained C
   candidate `void func_00282C90(double *a0, double a1) { *a0 = a1; }` emits
   an ordinary GPR-pair store (`sd $a1,0($a0)`), not the retail instruction.
   This is evidence that the routine contains handwritten VU0 code, not a C
   match. Keep the marker pending maintainer agreement before using ASM_FUNC;
   no completion credit is claimed. */
void func_00282C90(double *a0, double a1) { *a0 = a1; }
