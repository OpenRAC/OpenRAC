/* func_00282EB0 - 40 bytes, 28 places (the boot and every level), retail at
   0x00282EB0: ldc2 $1,0($a0) ; ldc2 $2,0($a1) ; 0x4BC000C3 ; 0x4BC2086C ; 0x4BC1086A ; 0x4B01083D ; 0x4B01184A ; 0x48240800 ; jr $ra ; mtc1 $a0,$f0.
   the body is VU0/GTE code: the 0x4A/0x4B words are COP2 special2 instructions and the ldc2/sdc2 are the VU's double load and store; this cc1 emits ldc1/sdc1 for a double and never a COP2 word.  Not matched in wave 12 (worker g1): no C source reproduces the body.
   What would unblock it: the GTE's own instructions. */
