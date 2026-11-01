/* GPT-6.1 Sol low tier, USA v1.01.
 * 252-byte residual extent, confirmed with current retail bytes. Previous body
 * returns at 00335268, restoring its 0x30 frame in the delay slot at 0033526C.
 * This extent contains positive stack adjustments and nops, followed by an
 * orphan gp-relative store at 00335368. No negative frame allocation or body
 * starts at +8: the old note mistook a positive adjustment for a prologue.
 * Next real prologue is 00335370. Requires catalogue investigation, not C.
 */
