/* GPT-6.1 Sol low tier, USA v1.01.
 * 444-byte residual extent, confirmed with current retail bytes. Previous body
 * returns at 00334548, restoring its 0x90 frame at 0033454C. This extent is
 * positive stack adjustments and nops, plus sw zero,0x14(a2) at 00334598.
 * The +8 instruction is another positive adjustment, not a prologue.
 * Next body starts at 00334710. Requires catalogue investigation, not C.
 */
