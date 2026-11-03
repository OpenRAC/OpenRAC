/* Candidate catalogue residue; no C match attempted.
 *
 * The boot extent is `addiu $sp,$sp,0x70; nop; sw $zero,-0x3FF8($gp)`.
 * It follows a completed return, its delay slot and a nop; the next frame
 * begins at 0x0031E6E8. It has no matching frame setup or return. A scan of the
 * loaded executable found no direct branch/jump, aligned literal pointer, or
 * nearby `lui` plus `addiu`/`ori` construction targeting this extent.
 *
 * This is evidence of a catalogue fragment, not a C function. Keep it pending
 * maintainer review while release-scoped residue handling is unresolved; do
 * not add it to FUNCTION_RESIDUES yet.
 */
