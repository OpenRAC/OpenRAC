/* Catalogue review pending: the 12-byte USA v1.01 extent at 0x00280660 is
 * `sw $zero,0x14($v1); nop; sw $zero,0x1C($a0); nop`, with no branch or
 * return before func_00280670 begins at 0x00280670. Verify control-flow
 * ownership before changing the generated catalogue. */
