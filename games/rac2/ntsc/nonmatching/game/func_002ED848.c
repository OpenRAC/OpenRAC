/* Catalogue review pending: the 36-byte USA v1.01 extent at 0x002ED848
 * contains `sw $t3,-0x3FE8($gp); nop; addiu $v0,$v0,0; nop;
 * daddu $v0,$zero,$zero; nop; sw $a0,0x34($v0); nop; sw $a0,0x3C($v0); nop`.
 * There is no branch or return in the extent. The following words at
 * 0x002ED870 form func_002ED870's load/return sequence. Verify control-flow
 * ownership before changing the generated catalogue.
 */
