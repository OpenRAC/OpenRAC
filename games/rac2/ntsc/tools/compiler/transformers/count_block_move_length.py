"""Count an R5900 block move's machine-description length in loop padding.

The retail assembler pads loops shorter than seven instructions.  An unaligned
or aligned block-move loop (`struct = struct' over 0x40 bytes or more) is one
movstrsi_internal2 insn for its loads and one movstrsi_internal3 insn for its
stores, each several words long.  Counting each as one word made every such
loop look five words long and gave it two NOPs that retail never has.
"""
from pathlib import Path
import sys

OLD = "\t  n += pat == TRAP_IF ? get_attr_length (insn) : 1;"
NEW = ("\t  /* Use the MD word count for a division-guard expansion and\n"
       "\t     for a block move's load or store group.  */\n"
       "\t  n += (pat == TRAP_IF\n"
       "\t\t|| (pat == PARALLEL\n"
       "\t\t    && GET_CODE (XVECEXP (PATTERN (insn), 0, 0)) == SET\n"
       "\t\t    && GET_MODE (SET_DEST (XVECEXP (PATTERN (insn), 0, 0))) == BLKmode))\n"
       "\t       ? get_attr_length (insn) : 1;")


def patch(path):
    path = Path(path)
    source = path.read_text(encoding="utf-8")
    start = source.index("void\nmips_r5900_pad_loops (first)")
    end = source.index("/* ASTRA P25:", start)
    fragment = source[start:end]
    if fragment.count(OLD) != 1:
        raise ValueError("expected the division-guard loop counter once, found %d" % fragment.count(OLD))
    path.write_text(source[:start] + fragment.replace(OLD, NEW) + source[end:], encoding="utf-8", newline="")


if __name__ == "__main__":
    patch(sys.argv[1])
    print("Block moves use their MD instruction length in post-DBR padding")
