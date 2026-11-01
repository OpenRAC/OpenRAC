/* func_00282E48 is 56 bytes at the boot and all 27 level placements. It
 * loads two vectors, subtracts xyz, squares and sums the components, computes
 * vsqrt through Q, and transfers the result to the scalar FPU return
 * register. COP2/VU0 instructions are required; the project compiler emits
 * none from C. No C attempt is appropriate. Keep INCLUDE_ASM pending
 * maintainer agreement for ASM_FUNC, as recorded in blocker 52.
 */
