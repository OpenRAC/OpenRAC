/* Candidate catalogue residue; no C match attempted.
 *
 * USA v1.01 extent: 12 bytes in the boot and levels 15, 19 and 26. At every
 * placement the preceding body returns in the two words before this row, and
 * this row is `sh $zero,0x1C($a0)`, `nop`, `addiu $sp,$sp,0x30`. The following
 * instruction is a nop and the next body begins with `addiu $sp,$sp,-0x10`.
 * The stack pop has no corresponding frame in this extent. Scans of each
 * loaded executable found no direct branch or jump to the row, no aligned
 * literal pointer to it, and no nearby `lui` plus `addiu`/`ori` construction
 * of its address.
 *
 * This is evidence of a catalogue fragment, not a C reconstruction. It remains
 * pending maintainer review because residue exclusion is not release-scoped;
 * do not add it to FUNCTION_RESIDUES until the cross-release audit mechanism
 * is resolved.
 */
