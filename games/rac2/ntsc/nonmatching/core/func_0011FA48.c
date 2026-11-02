/* from src/core/0011FA20.c: the row holds no code at all.
 *
 *   0x11FA48  addiu $sp,$sp,0x20  the dead word the retail leaves behind a
 *                                function's return: byte for byte the same
 *                                0x27BD0020 as the delay slot of exit's own
 *                                `j func_0011A840` at 0x11FA40 (its delay slot
 *                                is the word at 0x11FA44). A dead word of the
 *                                same kind opens the rows func_0012F998 (after
 *                                func_0012F948's `jr $ra` at 0x12F990) and
 *                                func_00132B88 (after func_00132B58's `jr $ra`
 *                                at 0x132B80), and the recorded func_0011FB8C
 *                                and func_00128F48 rows start the same way;
 *   0x11FA4C  0xCDCDCDCD          the linker's fill word between two objects.
 *
 * exit (symbol_addrs/usa-v2: `exit = 0x0011fa20`) is 40 bytes and ends in
 * that jump, so the row is the two words between exit and the next object
 * (func_0011FA50, the syscall stub). Neither of them is code: no jr $ra, no
 * branch, and 0xCDCDCDCD is an instruction no compiler emits.
 *
 * tools/check.py with a stub body confirms it:
 *
 *   tools/check.py func_0011FA48 --diff
 *     boot: instruction at +0x0 differs
 *       0011FA48  jr          $ra                          | addiu       $sp, $sp, 0x20
 *       0011FA4C  nop                                      | pref        0x0D, -0x3233($t6)
 *
 * What it wants is a tools/levels.py filler row of its own (the shape the
 * level rows func_L20_00444130 and func_L20_0036AE68 record), not a C body.
 * One stub compile was spent to read the two words back with the tool.
 */
