/* Candidate catalogue residue; no C match attempted.
 *
 * The boot extent is one store: `sw $zero,0($v0)`. It follows a completed
 * return, its delay slot and a nop; the next body begins with a new 0x360-byte
 * frame at 0x003380B0. A scan of the loaded executable found no direct
 * branch/jump, aligned literal pointer, or nearby `lui` plus `addiu`/`ori`
 * construction targeting this word.
 *
 * This is evidence of a catalogue fragment, not a C function. Keep it pending
 * maintainer review while release-scoped residue handling is unresolved; do
 * not add it to FUNCTION_RESIDUES yet.
 */
