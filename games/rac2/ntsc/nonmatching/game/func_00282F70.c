/* func_00282F70 is 72 bytes at the boot and all 27 level placements. The
 * body loads an input vector with lqc2, forms its xy squared length with
 * vmul.xy/vaddy.x, and uses vrsqrt, vwaitq and vmulq to scale the vector.
 * A zero test selects a VU0 zero-vector path. The result is written with
 * sqc2. The qualified compiler emits no VU0 instructions, so a C attempt
 * cannot match this body. Keep INCLUDE_ASM pending maintainer agreement for
 * ASM_FUNC, as recorded in blocker 52.
 */
