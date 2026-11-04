# Authored li.d controls for tests/test_inline_li_d.py (no game bytes).
	.set	noreorder
	.text
	.globl	tenth
	.ent	tenth
tenth:
	li.d	$5,1.00000001490116119385e-1
	jr	$31
	nop
	.end	tenth
	.size	tenth,.-tenth
	.globl	ten4
	.ent	ten4
ten4:
	li.d	$18,9.99999974737875163555e-5
	jr	$31
	nop
	.end	ten4
	.size	ten4,.-ten4
	.globl	simple
	.ent	simple
simple:
	li.d	$5,872.0
	jr	$31
	nop
	.end	simple
	.size	simple,.-simple
