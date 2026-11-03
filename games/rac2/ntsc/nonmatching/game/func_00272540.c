/* Candidate catalogue residue; no C match attempted.
 *
 * The boot extent is `daddu $v0,$a1,$zero; nop; addiu $sp,$sp,0x10`.
 * It follows a completed return and its delay slot; a new 0x30-byte frame
 * begins at 0x00272550. It has no matching frame setup or return. A scan of
 * the loaded executable found no direct branch/jump, aligned literal pointer,
 * or nearby `lui` plus `addiu`/`ori` construction targeting this extent.
 *
 * This is evidence of a catalogue fragment, not a C function. Keep it pending
 * maintainer review while release-scoped residue handling is unresolved; do
 * not add it to FUNCTION_RESIDUES yet.
 */
