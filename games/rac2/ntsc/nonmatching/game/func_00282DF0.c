/* func_00282DF0 is 48 bytes at the boot and all 27 level placements. The
 * decoded body loads a vector, squares and sums xyz, computes vsqrt through
 * Q, then transfers the result to the scalar FPU return register. COP2/VU0
 * instructions are required; the project compiler emits none from C. No C
 * attempt is appropriate. Keep INCLUDE_ASM pending maintainer agreement for
 * ASM_FUNC, as recorded in blocker 52.
 */
