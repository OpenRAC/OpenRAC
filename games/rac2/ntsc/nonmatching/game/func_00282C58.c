/* func_00282C58 - 16 bytes, 28 places (the boot and every level), retail at
   0x00282C58: 0x708510E8 ; 0x704610E8 ; jr $ra ; sra $v0,$v0,0.
   the 0x70xxxxxx words are MMI0 instructions, which belong to the MMI unit; this cc1 never emits them.  Not matched in wave 12 (worker g1): no C source reproduces the body.
   What would unblock it: the MMI unit's own instructions. */
