/* Candidate catalogue residue; no C match attempted.
 *
 * The boot extent is `addiu $v1,$zero,0x23; syscall 0` at 0x00131CB0. The
 * preceding body has already tail-jumped away at 0x00131CA8, with its delay
 * slot at 0x00131CAC. The actual `InstallLoadedOverlay` body begins at
 * 0x00131CB8 (symbol_addrs/usa-v2). A scan of the loaded executable found no
 * direct branch/jump, aligned literal pointer, or nearby `lui` plus
 * `addiu`/`ori` construction targeting this 8-byte extent.
 *
 * This is evidence of unreachable catalogue fragment data, not a C function.
 * Keep it pending maintainer review while release-scoped residue handling is
 * unresolved; do not add it to FUNCTION_RESIDUES yet.
 */
