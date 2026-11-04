/* func_002833B8 is 56 bytes at the boot and all 27 level placements. It
 * transfers the scalar argument into a VU0 register, forms three rows with
 * vmulx/vaddx and a fourth with vmove, then writes four quadwords with sqc2.
 * COP2/VU0 instructions are required; the project compiler emits none from C.
 * No C attempt is appropriate. Keep INCLUDE_ASM pending maintainer agreement
 * for ASM_FUNC, as recorded in blocker 52.
 */
