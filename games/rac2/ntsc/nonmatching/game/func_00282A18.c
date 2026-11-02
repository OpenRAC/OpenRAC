/* func_00282A18 - 16 bytes, 28 places (the boot and every level), retail at
   0x00282A18: 0x70802004 ; addi $v0,$zero,0x1E ; jr $ra ; sub $v0,$v0,$a0.
   the 0x70xxxxxx words are MMI0 instructions, which belong to the MMI unit; this cc1 never emits them.  Not matched in wave 12 (worker g1): no C source reproduces the body.
   What would unblock it: the MMI unit's own instructions. */
