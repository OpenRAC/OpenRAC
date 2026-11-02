/* func_00282C10 - 32 bytes, 28 places (the boot and every level), retail at
   0x00282C10: mfc1 $a0,$f12 ; 0x48A40800 ; 0x4A0103BD ; 0x4A0003BF ; 0x4B000060 ; 0x48240800 ; jr $ra ; mtc1 $a0,$f0.
   the body is VU0/GTE code: the 0x4A/0x4B words are COP2 special2 instructions and the ldc2/sdc2 are the VU's double load and store; this cc1 emits ldc1/sdc1 for a double and never a COP2 word.  Not matched in wave 12 (worker g1): no C source reproduces the body.
   What would unblock it: the GTE's own instructions. */
