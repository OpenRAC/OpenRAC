# Authored reorder-mode branches whose delay slot the compiler left to the
# assembler, in the shape the SDK compiler writes them (tests/test_swap_branch_slots.py).
	.text
	.set	noat

# A volatile store before a call, as in func_0012E378.
	.globl	store_call
	.ent	store_call
store_call:
	#.set	volatile
	sw	$4,0($5)
	#.set	novolatile
	jal	callee
	.end	store_call
	.size	store_call,.-store_call

# A volatile word load before a call, as in func_001253A8.
	.globl	load_call
	.ent	load_call
load_call:
	#.set	volatile
	lw	$4,0($5)
	#.set	novolatile
	jal	callee
	.end	load_call
	.size	load_call,.-load_call

# truncdisi2's two words before the return, as in func_0011C000.
	.globl	trunc_return
	.ent	trunc_return
trunc_return:
	dsll	$2,$4,32
	dsra	$2,$2,32
	j	$31
	.end	trunc_return
	.size	trunc_return,.-trunc_return

# Controls: the branch reads the register the previous instruction sets...
	.globl	dependent_return
	.ent	dependent_return
dependent_return:
	addiu	$2,$4,1
	j	$2
	.end	dependent_return
	.size	dependent_return,.-dependent_return

# ...and a branch that is itself a branch target.
	.globl	labelled_return
	.ent	labelled_return
labelled_return:
	addiu	$2,$4,1
$L1:
	j	$31
	.end	labelled_return
	.size	labelled_return,.-labelled_return
