/*
 * No C attempt: this called 40-byte routine is byte-identical at all 28
 * USA v1.01 placements and uses VU0/COP2 vector operations: vmulx.xyzw,
 * vmr32.xyzw, vaddw.x/y and sqc2. The project toolchain's C objects contain
 * no VU0 instructions. Direct calls reach it in every program; keep
 * INCLUDE_ASM pending maintainer agreement for ASM_FUNC.
 */
