/* func_002833F0 - 32 bytes, 28 places (the boot and every level), retail at
   0x002833F0: ldc2 $1,0($a1) ; 0x4A0068F8 ; 0x4821A001 ; sdc2 $20,0($a0) ; sdc2 $21,0x10($a0) ; sdc2 $22,0x20($a0) ; jr $ra ; nop.
   the body is VU0/GTE code: the 0x4A/0x4B words are COP2 special2 instructions and the ldc2/sdc2 are the VU's double load and store; this cc1 emits ldc1/sdc1 for a double and never a COP2 word.  Not matched in wave 12 (worker g1): no C source reproduces the body.
   What would unblock it: the VU's own instructions. */
