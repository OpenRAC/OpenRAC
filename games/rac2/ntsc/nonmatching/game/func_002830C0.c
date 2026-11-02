/* func_002830C0 is 48 bytes at the boot and all 27 level placements. It
 * loads the input vector and four matrix columns with lqc2, accumulates the
 * vector-matrix product with vmulax/vmadd*, and writes the result with sqc2.
 * COP2/VU0 instructions are required; the project compiler emits none from C.
 * No C attempt is appropriate. Keep INCLUDE_ASM pending maintainer agreement
 * for ASM_FUNC, as recorded in blocker 52.
 */
