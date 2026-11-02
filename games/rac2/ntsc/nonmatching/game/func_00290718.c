/* Candidate catalogue residue; no C match attempted.
 *
 * The boot extent is one store: `sw $v0,-0x3D14($gp)`. It follows a completed
 * return and its delay slot, then a nop. The next catalogue body at
 * 0x00290720 is a separate `jr $ra` with its own store in the delay slot. A
 * scan of the loaded executable found no direct branch/jump, aligned literal
 * pointer, or nearby `lui` plus `addiu`/`ori` construction targeting this word.
 *
 * This is evidence of a catalogue fragment, not a C function. Keep it pending
 * maintainer review while release-scoped residue handling is unresolved; do
 * not add it to FUNCTION_RESIDUES yet.
 */
