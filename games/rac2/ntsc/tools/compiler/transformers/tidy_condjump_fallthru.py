#!/usr/bin/env python3
"""Let flow delete a conditional jump to the next block in gcc/flow.c, as GCC 2.95 does.

delete_unreachable_blocks deletes a jump whose only edge leads to the next block, but this
tree (from the March 1999 trunk) only looks at an edge not marked fallthru. A conditional jump
to the next block has its two edges merged into one fallthru edge, so it stays until the jump
pass after sched2, which deletes it but not the insn that fed it (delete_computation deletes
only the jump once reload is done and sched2 runs): a dead `lh` at the entry of the levels'
rotated id-list walks, which retail never has. GCC fixed the test on 8 April 1999 ("Do not
require EDGE_FALLTHRU for an edge when tidying an edge which connects consecutive basic
blocks") and limited it to plain jumps on 4 August 1999; this is the test as the GCC 2.95
branch has it. flow2 then deletes the jump, and its life analysis the dead load
(func_L02_003FD070 and the other rotated walks)."""
import sys
from pathlib import Path

path = Path(sys.argv[1])
source = path.read_text(encoding="utf-8")
old = ("      /* We only need care for simple unconditional jumps, which implies\n"
       "\t a single successor.  */\n"
       "      if ((s = b->succ) != NULL\n"
       "\t  && s->succ_next == NULL\n"
       "\t  && s->dest == c\n"
       "\t  && ! (s->flags & EDGE_FALLTHRU))\n"
       "\ttidy_fallthru_edge (s, b, c);\n")
new = ("      /* We care about simple conditional or unconditional jumps with\n"
       "\t a single successor.\n"
       "\n"
       "\t If we had a conditional branch to the next instruction when\n"
       "\t find_basic_blocks was called, then there will only be one\n"
       "\t out edge for the block which ended with the conditional\n"
       "\t branch (since we do not create duplicate edges).\n"
       "\n"
       "\t Furthermore, the edge will be marked as a fallthru because we\n"
       "\t merge the flags for the duplicate edges.  So we do not want to\n"
       "\t check that the edge is not a FALLTHRU edge.  */\n"
       "      if ((s = b->succ) != NULL\n"
       "\t  && s->succ_next == NULL\n"
       "\t  && s->dest == c\n"
       "\t  /* If the last insn is not a normal conditional jump\n"
       "\t     (or an unconditional jump), then we can not tidy the\n"
       "\t     fallthru edge because we can not delete the jump.  */\n"
       "\t  && GET_CODE (b->end) == JUMP_INSN\n"
       "\t  && condjump_p (b->end))\n"
       "\ttidy_fallthru_edge (s, b, c);\n")
assert source.count(old) == 1, ("the fallthru test in delete_unreachable_blocks", source.count(old))
path.write_text(source.replace(old, new), encoding="utf-8", newline="")
