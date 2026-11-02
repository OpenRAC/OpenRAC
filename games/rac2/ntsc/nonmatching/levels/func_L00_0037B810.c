/* func_L00_0037B810 (4 bytes, 23 places) is not a function: it is the four bytes
 * `sw $v0, -0x3FF8($gp)` that the catalogue split off the end of the function before
 * it (func_L00_0037B7B0, 92 bytes, matched in L00_0037B6F0.c).
 *
 * Evidence:
 *   - the four bytes have no `jr $ra` and no way to return: they store the value
 *     still live in $v0, which only the preceding function can leave there;
 *   - in every one of the 23 places they follow that function's `jr $ra` / delay-slot
 *     store / `nop` (level 00: 0x0037b804 jr, 0x0037b808 sw $v0,0x3618($gp),
 *     0x0037b80c nop, 0x0037b810 sw $v0,-0x3FF8($gp));
 *   - in the four levels where the catalogue does not list them (08, 09, 19, 21, 30)
 *     the store is simply not there: level 30's function ends with
 *     `jr $ra ; sw $v0,0x4498($gp) ; nop` and nothing after it.
 *
 * No C can produce it: a plain function has no incoming $v0, and the body would have
 * to be a single store of a value nothing provides. The fix is in the catalogue
 * (tools/levels.py), not in C: the four bytes belong to the preceding function, or
 * the row should go.
 */
