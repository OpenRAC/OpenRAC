# A division at a line label right after a call, in the shape cc1 -g writes it
# (tests/test_div_label_after_call.py). The call and its delay slot sit in a
# compiler .set noreorder group; the label and the division follow in reorder mode.
	.text
	.set	noat

# Retail func_L00_0040A4A8 +0x200: jal, delay, nop, nop, div.s.
	.globl	label_after_call
	.ent	label_after_call
label_after_call:
	.set	noreorder
	.set	nomacro
	jal	callee
	move	$5,$21
	.set	macro
	.set	reorder

$LM1:
	div.s	$f12,$f0,$f20
	.end	label_after_call
	.size	label_after_call,.-label_after_call

# The same call with the division in its statement: no label, no padding
# (func_002C3280).
	.globl	no_label_after_call
	.ent	no_label_after_call
no_label_after_call:
	.set	noreorder
	.set	nomacro
	jal	callee
	mov.s	$f20,$f0
	.set	macro
	.set	reorder

	div.s	$f12,$f20,$f0
	.end	no_label_after_call
	.size	no_label_after_call,.-no_label_after_call

# A division inside a noreorder region still gets no new floor.
	.globl	label_in_noreorder
	.ent	label_in_noreorder
label_in_noreorder:
	.set	noreorder
	.set	nomacro
	jal	callee
	move	$5,$21
$LM2:
	div.s	$f12,$f0,$f20
	.set	macro
	.set	reorder
	.end	label_in_noreorder
	.size	label_in_noreorder,.-label_in_noreorder
