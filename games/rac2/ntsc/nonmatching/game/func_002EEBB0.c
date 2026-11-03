/* Catalogue review pending: the 20-byte USA v1.01 extent at 0x002EEBB0 is
 * `daddu $v0,$zero,$zero; nop; addiu $sp,$sp,0x140; nop;
 * addiu $sp,$sp,0xC0; nop`. It has no branch or return; func_002EEBC8 begins
 * immediately afterward. The stack restores appear to be an orphaned
 * epilogue fragment. Verify control-flow ownership before changing the
 * generated catalogue. */
