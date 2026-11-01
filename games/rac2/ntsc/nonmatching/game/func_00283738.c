/*
 * No C attempt: this 80-byte routine is present in the boot and 27 levels.
 * Its USA v1.01 body loads two 3x4 matrices with lqc2, multiplies and
 * accumulates their rows with VU0 vector operations, stores three result rows
 * with sqc2, and places the final store in the live jr-ra delay slot. All 27
 * level placements contain the lqc2 opcode. The compiler does not emit
 * COP2/VU0 operations from C. This supports handwritten-assembly review,
 * but keep INCLUDE_ASM until a maintainer agrees to ASM_FUNC.
 */
