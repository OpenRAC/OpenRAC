/* func_002837D0 is 64 bytes at the boot and all 27 level placements. It
 * loads two vectors, computes componentwise products, uses vopmula/vopmsub
 * for cross-product terms, accumulates a dot-product component, and stores
 * the result with sqc2. COP2/VU0 instructions are required; the project
 * compiler emits none from C. No C attempt is appropriate. Keep INCLUDE_ASM
 * pending maintainer agreement for ASM_FUNC, as recorded in blocker 52.
 */
