/* Unresolved catalogue-boundary hypothesis, not a C near miss.
   USA v1.01 places a 16-byte extent immediately after func_0012F948's completed
   return and stack-adjustment delay slot:

     0x0012F998  addiu $sp,$sp,0x10
     0x0012F99C  nop
     0x0012F9A0  sw    $a2,0x98($v0)
     0x0012F9A4  nop

   The next row, func_0012F9A8, begins its own load sequence. A current empty-C
   probe is 8 bytes and matches 0/1. Scanning the USA v1.01 boot found no direct
   branch/jump target in the row and no aligned loaded-word pointer to it. This
   supports the residue hypothesis but does not establish PAL or USA v2.00
   boundaries; keep the marker and catalogue row pending maintainer review.

   rac1-decomp carries the same bytes (at 0x0012BB78, its core's +0x3E20 shift)
   as a linker remnant, which is historical corroboration only. */
