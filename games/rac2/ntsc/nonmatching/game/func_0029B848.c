/* Catalogue review pending: func_0029B848 is listed as a 12-byte function in
 * USA v1.01. The retail words at 0x0029B848..0x0029B854 are
 * `daddu $v0,$zero,$zero; nop; addiu $sp,$sp,0x10; nop`; there is no return
 * within the catalogue extent, and the next catalogue function starts at
 * 0x0029B858. An isolated empty C stub compiles to 8 bytes (jr $ra; nop),
 * while the listed retail extent has 12 bytes. This is not enough by itself
 * to change the generated catalogue; a maintainer should review control flow
 * and the shared residue handling before any catalogue edit.
 */
