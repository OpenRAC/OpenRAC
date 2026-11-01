/* Catalogue review pending: the 20-byte USA v1.01 extent at 0x002ED9A8 is
 * `lw $v0,0xC($v1); nop; addiu $sp,$sp,0x20; nop;
 * addiu $sp,$sp,0x20; nop`. It has no branch or return, and the next function
 * func_002ED9C0 begins immediately after it. Verify control-flow ownership
 * before changing the generated catalogue. */
