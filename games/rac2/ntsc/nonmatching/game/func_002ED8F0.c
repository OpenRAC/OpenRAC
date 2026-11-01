/* Catalogue review pending: the 84-byte USA v1.01 extent at 0x002ED8F0 is
 * a run of loads and stack-pointer adjustments (including repeated positive
 * adjustments) ending with `addiu $sp,$sp,0x30; nop`. There is no branch or
 * return in the extent; ordinary game instructions resume at 0x002ED948.
 * Verify control-flow ownership before changing the generated catalogue.
 */
