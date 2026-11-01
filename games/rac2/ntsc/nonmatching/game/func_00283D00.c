/* func_00283D00 is 56 bytes at the boot and all 27 level placements. It
 * transfers two scalar inputs to VU0, divides through Q, converts the
 * quotient to integer and back, subtracts its truncated part, multiplies by
 * the divisor, then transfers the result from VU0 to the scalar FPU return
 * register. COP2/VU0 instructions are required; the project compiler emits
 * none from C. No C attempt is appropriate. Keep INCLUDE_ASM pending
 * maintainer agreement for ASM_FUNC, as recorded in blocker 52.
 */
