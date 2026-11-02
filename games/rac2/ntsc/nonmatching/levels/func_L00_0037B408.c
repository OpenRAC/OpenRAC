/* The retail body of func_L00_0037B408 is four bytes: a single
 * `sw $v0, -0x3FF8($gp)`.  It has no `jr $ra` and no branch of its own,
 * and no branch in any of the 27 programs reaches it (checked with
 * scratch/findref.py): it is dead code between two alignment nops.
 *
 * No C produces it.  A plain store of a constant needs the constant's own
 * `li` (the value has to be in a register) and the function needs its
 * `j $31`, so the smallest body this cc1 emits for such a store is
 * `li $v0,1 ; j $31 ; sw $v0,word($gp)` - 12 bytes, where retail has 4.
 * The value in $v0 is the previous function's leftover here, so it cannot
 * be named either.
 *
 * The candidate below is the closest plain-C shape: the same word, stored
 * with the same gp-relative offset (the word is at 0x001AAF08, $gp-0x3FF8),
 * but 12 bytes long.
 */

extern int D_001AAF08 __attribute__((sda));

void func_L00_0037B408(void)
{
    D_001AAF08 = 1;
}
