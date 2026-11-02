/* Candidate catalogue residue; no C match attempted.
 *
 * The boot extent is one word: `daddu $v0,$zero,$zero`. It follows a completed
 * return and its delay slot, then one linker nop; the next body starts at
 * 0x0011B328. A scan of the loaded executable found no direct branch/jump,
 * aligned literal pointer, or nearby `lui` plus `addiu`/`ori` construction
 * targeting 0x0011B320.
 *
 * This is evidence of a catalogue fragment, not a C function. Keep it pending
 * maintainer review while release-scoped residue handling is unresolved; do
 * not add it to FUNCTION_RESIDUES yet.
 */
