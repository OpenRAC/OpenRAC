/* Catalogue review pending: the 4-byte USA v1.01 extent at 0x002ED570 is
 * `lw $v0,0x24($v1)`; the following word at 0x002ED574 is `nop`, and the next
 * catalogue function begins at 0x002ED578. The listed extent contains no
 * branch or return and is consistent with a split instruction fragment, but
 * the surrounding function/control-flow ownership must be verified before
 * changing the generated catalogue. */
