/* Unresolved catalogue-tail hypothesis, not an exact C match. USA v1.01
   has a 52-byte row at 0x0012FB10:

     sw $v1,0x70($v0); nop; sw $a1,0xD8($v0); nop;
     lw $v0,0xCC($v1); nop; lw $v0,0xD0($v1); nop;
     addiu $v0,$v0,0xB4; nop; addiu $v0,$v0,0xB4; nop;
     sd $a1,0xF0($v1)

   It follows a completed return, its stack-adjustment delay slot and one `nop`,
   then falls into the next row. A scan of catalogued USA v1.01 boot functions
   found no direct branch/jump target into it; no aligned loaded-word pointer
   to its range was found. Earlier C probes (24 bytes and 52 bytes) are retained
   historical measurements, not exact matches. rac1-decomp records the same
   bytes as a linker remnant; that is provenance, not independent boundary proof.

   The row appears in all three release catalogues, but PAL and USA v2.00 retail
   dumps are unavailable in this checkout, so leave the source/catalogue intact
   pending release-qualified maintainer review. */
