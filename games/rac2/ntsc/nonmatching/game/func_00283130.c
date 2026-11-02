/* func_00283130 - 24 bytes, 28 places (the boot and every level), retail at
   0x00283130: 0x70A02D88 ; 0x70052C3F ; 0x48A50800 ; 0x4BE1093C ; jr $ra ; sdc2 $1,0($a0).
   the 0x70xxxxxx words are MMI0 instructions, which belong to the MMI unit; this cc1 never emits them, and the body also holds a VU word.  Not matched in wave 12 (worker g1): no C source reproduces the body.
   What would unblock it: the MMI and VU units' own instructions. */
