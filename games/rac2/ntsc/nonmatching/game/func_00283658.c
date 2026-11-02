/* func_00283658 - 32 bytes, 28 places (the boot and every level), retail at
   0x00283658: lq $1,0($a1) ; lq $2,0x10($a1) ; lq $3,0x20($a1) ; sq $1,0($a0) ; sq $2,0x10($a0) ; sq $3,0x20($a0) ; jr $ra ; sdc2 $0,0x30($a0).
   the body is the 48-byte copy of func_00283678 plus a VU0 double store at +0x30, and this cc1 stores a double through a GPR pair (sd) or a library call.  Not matched in wave 12 (worker g1): no C source reproduces the body.
   What would unblock it: a source that stores the double in the VU's register 0. */
