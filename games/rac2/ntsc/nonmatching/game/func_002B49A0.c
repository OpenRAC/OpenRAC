/* Candidate catalogue residue; no C match attempted.
 *
 * The boot extent is `addiu $sp,$sp,0x20; nop; slt $v0,$zero,$v0`. It follows
 * a completed return and its delay slot; a separate `jr $ra` row begins at
 * 0x002B49B0. The extent has no frame setup or return. A scan of the loaded
 * executable found no direct branch/jump, aligned literal pointer, or nearby
 * `lui` plus `addiu`/`ori` construction targeting 0x002B49A0.
 *
 * This is evidence of a catalogue fragment, not a C function. Keep it pending
 * maintainer review while release-scoped residue handling is unresolved; do
 * not add it to FUNCTION_RESIDUES yet.
 */
