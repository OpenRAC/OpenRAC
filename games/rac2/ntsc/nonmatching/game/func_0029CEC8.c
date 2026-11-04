/* Catalogue review pending: the 12-byte USA v1.01 extent at 0x0029CEC8 is
 * `sw $a0,-0x4000($gp); nop; addiu $sp,$sp,0x10; nop`. It has no branch or
 * return, and func_0029CED8 begins immediately after it. Verify control-flow
 * ownership and shared residue handling before changing the generated row. */
