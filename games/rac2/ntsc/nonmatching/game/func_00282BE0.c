/* func_00282BE0 - 40 bytes, 28 places (the boot and every level), retail at
   0x00282BE0: ldc2 $1,0($a0) ; 0x4A0002FF ; 0x4BC109FF ; 0x4A0002FF (x4) ; cfc2 $v0,18 ; jr $ra ; andi $v0,$v0,0x3F.
   the body is VU0/GTE code: the 0x4A/0x4B words are COP2 special2 instructions and the ldc2/sdc2 are the VU's double load and store; this cc1 emits ldc1/sdc1 for a double and never a COP2 word.  Not matched in wave 12 (worker g1): no C source reproduces the body.
   What would unblock it: the GTE's own instructions; a C source for the GTE's RTPS/NCLIP routine. */
