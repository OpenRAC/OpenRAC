/* func_L14_0035F510 (288 bytes, 1 place) -- NOT MATCHABLE, the catalogue row is eight bytes
   too wide at its start; the fix belongs in tools/levels.py.

   The row runs from 0x0035F510 to 0x0035F630, but the body starts at 0x0035F518.  The eight
   bytes before it are what the function before it left behind:

     0035F500  ld    $ra,0x48($sp)     <- func_002DB7F0 (the boot function at 0x0035F378)
     0035F504  jr    $ra
     0035F508  addiu $sp,$sp,0x50       (its frame restore, in the delay slot)
     0035F50C  nop                     <- alignment padding
     0035F510  sw     $t3,0x40($v1)    <- the leftover word: a store through $v1, which
     0035F514  nop                          no code has set at that point
     0035F518  addiu  $sp,$sp,0x20     <- the row's real start: the prologue
     ...
     0035F628  jr    $ra
     0035F62C  daddu  $v0,$zero,$zero  (the delay slot)

   A function cannot begin with a store through an undefined register, and the compiler
   never emits one, so the row cannot match at any spelling: check.py would say
   "280 bytes, expected 288".  This is the shape docs/LEVELS.md describes for the boot
   residues ("a four-byte extent right after a body that ends with jr $ra and its delay
   slot is the word that body left behind"), here eight bytes wide because the leftover
   word is followed by a nop.

   The body itself is 0x0035F518..0x0035F630, 280 bytes, and reads as:

     - the frame is 0x20 and there is no callee-saved register to spill;
     - it walks an array of indices at 0x00139948 (an absolute address in the boot's
       data) and for each index writes a twelve-byte record at 0x0021D630: the two
       halfwords of the index's entry in the table at 0x00291720 (twelve-byte entries,
       the halfword at +0 and the one at +4) go to the record's +0 and +8, the constant
       3 to its +2 and the pointer 0x0028D1D0 to its +4; the record pointer advances by
       twelve and the index pointer by four;
     - the loop counter is incremented twice per round (once in the body, once in the
       branch-likely delay slot), and the loop ends when the counter reaches 28 or when
       the next index is zero;
     - after the loop it clears the halfword at the record's end (0x0021D630 + count*12),
       clears the word at self+0x40 and sets the 0x8000 bit of the word at self+0x30;
     - it then looks the object's own entry up in the list at 0x001AA770 (read through
       $gp, so the body needs the -g8 sibling) and stores the index in the word at
       self+0x40, with the comparisons against the word at 0x001C9D10.

   The fix belongs in tools/levels.py, like the three measured Aranos corrections
   (docs/LEVELS.md): the row should start at 0x0035F518 and be 280 bytes long.  The
   level's next row, func_002DBAA8, already starts at 0x0035F630, so only the start of
   this row moves.  Nothing else in the level changes. */
