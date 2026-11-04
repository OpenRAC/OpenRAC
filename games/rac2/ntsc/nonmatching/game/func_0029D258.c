/* func_0029D258 - handwritten assembly candidate, USA v1.01.
 *
 * The retail body is 320 bytes at the boot and 27 level placements. Decoding
 * the boot body with the R5900 instruction set shows COP2/VU0 operations
 * (`lqc2`, `qmtc2`, `vmulax`, `vmadday`, `qmfc2`) interleaved with MMI packed
 * integer operations (`pextlb`, `pextlh`, `pmulth`, `pmfhl.sh`, `ppacb`,
 * `ppach`). These are the renderer's vector transform and packed colour
 * calculations; the game's C compiler does not emit the COP2 instruction
 * sequence. This is evidence for maintainer review as handwritten assembly,
 * not a C match. The INCLUDE_ASM marker remains in src/game/0029BCC8.c until
 * that review accepts an ASM_FUNC classification.
 */
