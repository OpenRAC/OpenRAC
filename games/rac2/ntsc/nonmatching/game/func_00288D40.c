/* Catalogue/control-flow review pending: func_00288D40 is listed as 20 bytes
 * in USA v1.01, but the words at 0x00288D40..0x00288D54 are
 * `daddu $v0,$zero,$zero; nop; addiu $sp,$sp,0x10; nop;
 * addiu $sp,$sp,0x10; nop`. There is no return or branch in the extent, and
 * the next catalogue function, func_00288D58, begins immediately afterward
 * with a load/return sequence. An isolated empty C stub is 8 bytes and does
 * not explain the region. Do not edit the generated catalogue without a
 * maintainer review of its control flow and the shared residue handling.
 */
