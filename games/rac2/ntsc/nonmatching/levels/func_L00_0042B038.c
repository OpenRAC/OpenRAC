/* func_L00_0042B038 (348 bytes, 7 places) - the retail holds THREE `bc1fl`, and this cc1
 * cannot emit that form.
 *
 * Retail at 0x0042b038 (level 00): the three float compares
 *     c.ole.s $f0,$f21 ; bc1fl +0x?? ; lbu $v0,0x3($s0)
 *     c.ole.s $f0,$f21 ; bc1fl +0x?? ; lbu $v0,0x3($s0)
 *     c.ole.s $f0,$f21 ; bc1f  +0x?? ; lwc1 $f20,0x10($sp)
 * use the branch-LIKELY form of the floating-point branch (bc1fl), whose delay slot is
 * annulled when the branch is taken. The `bc1f` at +0x90 (the fourth compare) is the
 * ordinary form.
 *
 * `bc1fl`/`bc1tl` are the -mbranch-likely forms of `bc1f`/`bc1t`; this cc1 rejects the
 * option ("cc1: Invalid option `branch-likely'"), so the form is out of reach whatever
 * the source says. Measured with scratch/wslcmp.py on a two-line probe: the same
 * comparison gives `bc1f` without the option.
 *
 * The rest of the body reads as four calls to func_35F8D0 (the last one with five zero
 * arguments), three calls to func_2EE048 on the 16-byte block at +0x10 of the frame, the
 * two calls to func_2EF240/func_2EF250 around the 0.5 blend, and the byte flag at +0x3
 * of the sub-object. It is a plain C body apart from the three `bc1fl`.
 */
