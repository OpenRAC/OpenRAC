/* func_00283098 - 40 bytes, 28 places (the boot and every level), retail at
   0x00283098: ldc2 $5,0($a1) ; ldc2 $1,0($a2) ; ldc2 $2,0x10($a2) ; ldc2 $3,0x20($a2) ; 0x4BE509BC ; 0x4BE510BD ; 0x4BE518BE ; 0x4BE5018B ; jr $ra ; sdc2 $6,0($a0).
   the body is VU0/GTE code: the 0x4A/0x4B words are COP2 special2 instructions and the ldc2/sdc2 are the VU's double load and store; this cc1 emits ldc1/sdc1 for a double and never a COP2 word.  Not matched in wave 12 (worker g1): no C source reproduces the body.
   What would unblock it: the VU's own instructions. */
