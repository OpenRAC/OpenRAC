/* from src/levels/shared/L00_002D7940.c: not a function. The 4-byte row at
 * L00:0x002DA390 holds one word, `sw $zero,0x4($v0)`, and nothing else: no
 * prologue, no `jr $ra`, and it reads $v0, which it never sets. The body
 * before it (func_L00_002DA1E0, 420 bytes) ends at 0x002DA380 with
 * `jr $ra ; addiu $sp,$sp,0x80`; 0x002DA384, 0x002DA388 and 0x002DA38C are the
 * linker's padding, and the next real body starts at 0x002DA398
 * (`addiu $sp,$sp,-0x100`). So the word at 0x002DA390 is a stray fragment the
 * catalogue split off, not an entry point - the same defect class as
 * func_L00_002EC230 (see nonmatching/levels/func_L00_002EC230.c): a
 * tools/levels.py extent fix. Data for the maintainers, nothing to attempt
 * here. */
