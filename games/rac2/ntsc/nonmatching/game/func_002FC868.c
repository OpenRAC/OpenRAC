/* Candidate catalogue residue; no C match attempted.
 *
 * The boot extent is one GP-relative store: `sw $v0,-0x3FF8($gp)`. It follows
 * the preceding body's completed return and delay slot, then a nop; the next
 * body begins at 0x002FC870. A scan of the loaded executable found no direct
 * branch/jump, aligned literal pointer, or nearby `lui` plus `addiu`/`ori`
 * construction targeting this word.
 *
 * This is evidence of a catalogue fragment, not a C function. Keep it pending
 * maintainer review while release-scoped residue handling is unresolved; do
 * not add it to FUNCTION_RESIDUES yet.
 */
