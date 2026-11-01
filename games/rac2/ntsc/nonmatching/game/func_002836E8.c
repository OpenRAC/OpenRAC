/*
 * No C attempt: this 80-byte routine is present in the boot and 27 levels.
 * Its USA v1.01 body loads three vector rows with lqc2, performs VU0 vector
 * additions/subtractions, stores three results with sqc2, and returns with a
 * nop delay slot. All 27 level placements contain the lqc2 opcode. The
 * compiler does not emit COP2/VU0 operations from C. This supports
 * handwritten-assembly review, but keep INCLUDE_ASM until a maintainer agrees
 * to ASM_FUNC.
 */
