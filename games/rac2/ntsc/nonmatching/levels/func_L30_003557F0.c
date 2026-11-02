/* No attempt: the row starts with a linker remnant. The 288-byte extent at 0x003557F0
   begins

       sw $t3,0x40($v1)  /  nop  /  addiu $sp,$sp,0x20  /  nop

   - the first word is the residue the previous body left behind (it writes through a
   register the extent never sets) and the next two are the linker's fill between two
   bodies, so the real body is the 272 bytes at 0x00355800, from `lui $a1,0x14` to the
   body's end at 0x00355910. The previous row (func_L16_004051A8) ends cleanly with
   `jr $ra` at 0x003557E4 and its delay slot `addiu $sp,$sp,0x50` at 0x003557E8, so the
   four leading words belong to neither body. A tools/levels.py split is what unblocks
   it; that is not a worker's to make.

   For whoever picks it up: the body takes one pointer argument, reads a word at
   -0x66B8 of a 0x14xxxx table (0x14B948), walks a 24-word list at 0x29DEF0 in 12-word
   strides and writes the kind byte (3) and the two halfwords of each record, then clears
   a bit at +0x30 of the argument and compares the object's id against the one at
   0x1DA3A0. It is a leaf: no frame, no `jr $ra` of its own in the extent. */
