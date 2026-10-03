#!/usr/bin/env python3
"""
Put the second half of a final int truncation into the return's delay
slot, as retail's compiler did.

A function that returns a 64-bit value as int ends with gcc's truncdisi2,
`dsll R,S,32` then `dsra R,R,32`. Our compiler emits that pair as one
insn two instructions long, which reorg may not put in a delay slot, so
the final `j $31` goes out unfilled. Our assembler never moves an
instruction into a slot, so the result is `dsll; dsra; j $31; nop`.
Retail has `dsll; j $31; dsra` wherever a function ends this way
(func_0012AAA8, func_00119EA8), and never the unfilled form: its
compiler had the pair as two insns.

This makes that one rewrite: for a function's final `j $31` left
unfilled in reorder mode, directly preceded by `dsll R,S,32` /
`dsra R,R,32`, the dsra moves into the slot under noreorder.

usage: python tools/fix_trunc_slot.py IN.s OUT.s
"""
import re
import sys


def is_code(line: str) -> bool:
    s = line.strip()
    return bool(s) and not s.startswith((".", "#")) and not s.endswith(":")


def main() -> None:
    src, dst = sys.argv[1:3]
    lines = open(src).read().split("\n")
    out, n, reorder = [], 0, True
    for line in lines:
        s = line.strip()
        if s in (".set\tnoreorder", ".set noreorder"):
            reorder = False
        elif s in (".set\treorder", ".set reorder"):
            reorder = True
        if reorder and re.match(r"^\s*j\s+\$31\s*$", line):
            k = len(out) - 1
            while k >= 0 and not is_code(out[k]):
                k -= 1
            j = k - 1
            while j >= 0 and not is_code(out[j]):
                j -= 1
            dsra = re.match(r"^\s*dsra\s+(\$\d+),(\$\d+),32\s*$", out[k].split("#")[0]) if k >= 0 else None
            dsll = re.match(r"^\s*dsll\s+(\$\d+),(\$\d+),32\s*$", out[j].split("#")[0]) if j >= 0 else None
            if dsra and dsll and dsra.group(1) == dsra.group(2) == dsll.group(1):
                slot = out.pop(k)
                out += ["\t.set\tnoreorder", "\t.set\tnomacro", line, slot,
                        "\t.set\tmacro", "\t.set\treorder"]
                n += 1
                continue
        out.append(line)
    open(dst, "w").write("\n".join(out))
    print(f"fix_trunc_slot: {n} truncation(s) moved into the return slot {src} -> {dst}")


if __name__ == "__main__":
    main()
