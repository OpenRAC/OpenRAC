/* func_L06_00396268 (288 bytes, 1 place) -- NOT MATCHABLE, the catalogue row is sixteen
   bytes too wide at its start; the fix belongs in tools/levels.py.

   The row runs from 0x00396268 to 0x00396388, but the body starts at 0x00396278.  The
   four words before it belong to the function before it:

     00396258  ld    $ra,0x48($sp)   <- func_002DB7F0 (the boot function at 0x002DB7F0)
     0039625C  jr    $ra
     00396260  addiu $sp,$sp,0x50      (its frame restore, in the delay slot)
     00396264  nop                    <- alignment padding
     00396268  sw     $t3,0x40($v1)   <- the leftover: a store through $v1 and $t3,
     0039626C  nop                         which no code has set at that point
     00396270  addiu  $sp,$sp,0x20      and the restore of a frame this body never took
     00396274  nop
     00396278  lui    $a1,0x14        <- the row's real start: the body, which has no
     0039627C  daddu  $t2,$a0,$zero      frame of its own (it ends with `jr $ra` and
     ...                                 the delay slot `daddu $v0,$zero,$zero`)
     00396380  jr    $ra
     00396384  daddu  $v0,$zero,$zero

   A function cannot begin with a store through an undefined register and with a
   frame restore it never took, and the compiler never emits one, so the row cannot
   match at any spelling: check.py would say "272 bytes, expected 288".  This is the
   shape docs/LEVELS.md describes for the boot residues ("a four-byte extent right
   after a body that ends with jr $ra and its delay slot is the word that body left
   behind"), here sixteen bytes wide because the leftover word is followed by a nop and
   by the frame restore of the body before it.  The same family is recorded for
   func_L09_00352150 and func_L14_0035F510 in this file's neighbours.

   The evidence that the address is not an entry point:
   - nothing in level 06 branches to 0x00396268: the whole of the level's text was
     scanned for branch and jump targets and none of them reaches it;
   - the boot holds the same 288 bytes as func_002DB988 (0x002DB988, 288 bytes
     according to config/boot/functions.tsv), with the same first instruction
     `sw $t3,0x40($v1)`, and the boot's text was scanned the same way: nothing
     branches to 0x002DB988 either.

   What the body does, for whoever checks the catalogue: it reads the word at
   0x00139948 (a boot global) as a count, fills a table of twelve-byte records at
   0x0021D970 from the entries of the table at 0x002957A0 (twelve-byte entries, the
   halfword at +0 and the one at +4), stops after twenty-eight entries, zeroes the
   last record, clears the word at +0x40 of its argument, sets bit 0x8000 of the word
   at +0x30, and then walks the list at the boot global 0x001A6770 (read through $gp,
   so the body needs a -G8 unit) looking for the value the argument's state holds at
   0x001CA050, storing the index in the word at +0x40.

   The fix belongs in tools/levels.py, like the three measured Aranos corrections
   (docs/LEVELS.md): the row should start at 0x00396278 and be 272 bytes long.  The
   level's next row, func_002DBAA8, already starts at 0x00396388, so only the start
   of this row moves. */
