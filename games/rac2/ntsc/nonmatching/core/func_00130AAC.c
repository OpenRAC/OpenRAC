/* Candidate catalogue residue; no C match attempted.
 *
 * The boot extent is one `pref` instruction at 0x00130AAC. The preceding body
 * tail-jumps at 0x00130AA4 with its delay slot at 0x00130AA8; a new frame
 * prologue begins at 0x00130AB0. A scan of the loaded executable found no
 * direct branch/jump, aligned literal pointer, or nearby `lui` plus
 * `addiu`/`ori` construction targeting this word.
 *
 * This is evidence of a catalogue fragment, not a C function. Keep it pending
 * maintainer review while release-scoped residue handling is unresolved; do
 * not add it to FUNCTION_RESIDUES yet.
 */
