# Authored cases for tests/test_hazard_labels.py (no game code), one function per section.
# Each places a label next to a coprocessor transfer or compare; the test assembles this
# text with and without `.set rac2_hazard_labels' and compares the branch targets and words.

# Changed: a forward label in front of the nop an mtc1 needs before its reader.
	.section .text.forward_transfer,"ax",@progbits
	.set	noreorder
	b	$L1
	nop
	.set	reorder
	mtc1	$4,$f0
$L1:
	add.s	$f2,$f0,$f1
	.set	noreorder
	j	$31
	nop
	.set	reorder

# Changed: a forward label in front of the nop a compare needs before its branch, flushed by
# the `.set noreorder' that opens the branch.
	.section .text.forward_compare,"ax",@progbits
	.set	noreorder
	b	$L2
	nop
	.set	reorder
	c.le.s	$f1,$f0
$L2:
	.set	noreorder
	bc1fl	$L3
	nop
	.set	reorder
$L3:
	.set	noreorder
	j	$31
	nop
	.set	reorder

# Changed: a label reached only backward also stays in front of the nop.
	.section .text.backward_transfer,"ax",@progbits
	mtc1	$4,$f0
$L4:
	add.s	$f2,$f0,$f1
	.set	noreorder
	bc1t	$L4
	nop
	j	$31
	nop
	.set	reorder

# Control: a loop head after the alignment fill that follows an mtc1, whose first
# instruction does not read the register.
	.section .text.aligned_loop,"ax",@progbits
	mtc1	$4,$f20
	.p2align 3
$L5:
	lw	$2,0($5)
	.set	noreorder
	bne	$2,$0,$L5
	addu	$5,$5,4
	j	$31
	nop
	.set	reorder

# Control: a forward label after an instruction with no coprocessor hazard.
	.section .text.no_hazard,"ax",@progbits
	.set	noreorder
	b	$L6
	nop
	.set	reorder
	addu	$2,$4,$5
$L6:
	addu	$2,$2,$2
	.set	noreorder
	j	$31
	nop
	.set	reorder

# Control: a forward label after an mtc1 whose next instruction does not read it.
	.section .text.forward_no_reader,"ax",@progbits
	.set	noreorder
	b	$L7
	nop
	.set	reorder
	mtc1	$4,$f0
$L7:
	lw	$2,0($5)
	add.s	$f2,$f0,$f1
	.set	noreorder
	j	$31
	nop
	.set	reorder

# Control: the nop that `.set noreorder' flushes after an mtc1 is dropped again when the
# branch does not read the register; the label stays on the branch.
	.section .text.flushed_no_reader,"ax",@progbits
	.set	noreorder
	b	$L11
	nop
	.set	reorder
	mtc1	$4,$f0
$L11:
	.set	noreorder
	beq	$2,$0,$L12
	nop
	.set	reorder
	add.s	$f2,$f0,$f1
$L12:
	.set	noreorder
	j	$31
	nop
	.set	reorder

# Control: mfc1 is interlocked on the R5900 and gets no nop.
	.section .text.transfer_from_fpu,"ax",@progbits
	.set	noreorder
	b	$L8
	nop
	.set	reorder
	mfc1	$2,$f0
$L8:
	addu	$3,$2,$2
	.set	noreorder
	j	$31
	nop
	.set	reorder

# Control: the division floor already keeps its label in front of the padding.
	.section .text.division,"ax",@progbits
	.set	noreorder
	b	$L9
	nop
	.set	reorder
	mtc1	$4,$f0
$L9:
	div.s	$f2,$f1,$f0
	.set	noreorder
	j	$31
	nop
	.set	reorder

# Control: a loop label reached backward with no hazard.
	.section .text.backward_no_hazard,"ax",@progbits
	lw	$2,0($4)
$L10:
	addu	$2,$2,-1
	.set	noreorder
	bne	$2,$0,$L10
	nop
	j	$31
	nop
	.set	reorder
