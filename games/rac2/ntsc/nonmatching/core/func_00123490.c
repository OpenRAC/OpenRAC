/* Not a function: the 44 bytes the catalogue calls func_00123490 are linker
   padding between two real functions of the SDK's libgcc.a fp-bit.o, and no
   C can be placed over them.

   That object holds __pack_f (0x001232F0, 268 bytes), __unpack_f (0x00123400,
   144 bytes) and __make_fp (0x001234C0, 44 bytes); each starts on a 16-byte
   boundary, so 48 bytes lie between __unpack_f's end (0x00123490) and
   __make_fp's start (0x001234C0).  The last four of them (0x001234BC..
   0x001234C0) are zero padding; the 44 the catalogue takes are the rest of
   the linker's fill: `daddu $v0,$a2,$zero` / nop, `addiu $sp,$sp,0x60` / nop,
   `addiu $sp,$sp,0x60` / nop, `daddu $v0,$zero,$zero` / nop,
   `addiu $sp,$sp,0x50` / nop, `addiu $sp,$sp,0x30`.

   Evidence: the extent has no `jr $ra`, no branch and no caller (nothing in
   the boot jumps to 0x00123490), and the SDK compiler, given GCC's own
   gcc/config/fp-bit.c with -DFLOAT -DFLOAT_BIT_ORDER_MISMATCH -DNO_DENORMALS
   -DUS_SOFTWARE_GOFAST -DFINE_GRAINED_LIBRARIES -DL_pack_sf -DL_unpack_sf
   -DL_make_sf, reproduces __pack_f (268 bytes) and __unpack_f (144 bytes)
   byte for byte and __make_fp (44 bytes) instruction for instruction - its
   one call word is the relocation to __pack_f, where retail's call goes -
   and emits no fourth function.

   The fix belongs in the catalogue (a `tools/levels.py` extent fix), not in
   C: nothing this file could contain would compile to those bytes. */
