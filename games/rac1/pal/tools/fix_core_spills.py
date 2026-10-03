#!/usr/bin/env python3
"""
Rewrites callee-saved register spills in compiler-generated assembly from
128-bit quadword (`sq`/`lq`) to 64-bit doubleword (`sd`/`ld`), which is
what retail's `core_text` segment uses.

Why this exists
---------------
Retail's two segments were built by different SN sub-builds, and neither
available compiler reproduces `core_text` on its own:

  v1.36  spills $ra as `sd` but $s0-$s7 as `sq`, and lays the save slots
         out MIRRORED versus retail ($31 at offset 0, s-regs descending)
  v1.14  lays the save slots out EXACTLY as retail does ($16 at offset 0,
         s-regs ascending, $31 at the top) but spills everything as `sq`

So retail's `core_text` == v1.14's frame layout + `sd`/`ld` mnemonics.
Because the layout already matches, this is a pure mnemonic substitution:
the frame size, the slot stride (16 bytes even for 8-byte stores, which
is what retail does) and every offset stay exactly as v1.14 emitted them.
No offset arithmetic is involved, which is what makes this safe.

Verified against retail: `func_001138B8` spills $16/$17/$18/$19/$20/$31
with `sd` at 0/16/32/48/64/80 in a 0x60 frame — 16-byte stride, 8-byte
stores — and v1.14 produces that same layout for an equivalent shape.

Scope guard
-----------
Only `$sp`-relative spills of callee-saved registers ($16-$23, $30, $31)
are touched. A `sq`/`lq` against any other base register, or of any other
register, is left alone: those are genuine 128-bit memory operations in
the source (block copies, quadword clears), not register spills, and
narrowing them would silently change behaviour.

Address guard
-------------
`core_text` is not one translation unit. Everything from 0x12DB18 to the
end of the segment spills with `sq`, exactly like the `text` segment, and
everything below it spills with `sd`. Measured over the whole of retail's
core_text disassembly the split is clean -- 290 functions below the
boundary use `sd`, 49 at or above it use `sq`, and not one function sits
on the wrong side.

The two apparent counterexamples are not spills at all:

  func_0011D6D4  `sq $1, 0x10($26)` and friends -- handwritten assembly
                 saving the register file against $k0, and already marked
                 as handwritten in the disassembly
  func_0012E1B8  `sd $18, %lo(D_0015EDD8)($1)` -- a store to a global,
                 not a $sp-relative spill

So the boundary is a translation-unit boundary that survived into the
link: the same kind of fact as core_text and text having been built by
different sub-builds, and the same kind of authority. Functions at or
above it are left alone.
"""
import re
import sys

# First address of core_text's `sq` region; see "Address guard" above.
SQ_REGION_START = 0x12DB18

LABEL = re.compile(r"^(func_([0-9A-Fa-f]{8})):")

# Callee-saved GPRs: $s0-$s7, plus $fp/$s8 and $ra. Both the numeric and
# the symbolic spellings appear depending on how the compiler emits them.
SAVED = {f"${n}" for n in list(range(16, 24)) + [30, 31]}
SAVED |= {f"$s{n}" for n in range(9)} | {"$ra", "$fp", "$sp"} - {"$sp"}

SPILL = re.compile(
    r"^(?P<lead>\s*)(?P<op>sq|lq)(?P<gap>\s+)(?P<reg>\$\w+)\s*,\s*"
    r"(?P<off>-?\d+|0x[0-9A-Fa-f]+)\((?P<base>\$sp|\$29)\)(?P<tail>.*)$"
)
NARROW = {"sq": "sd", "lq": "ld"}


def rewrite(text: str) -> tuple[str, int]:
    out, n = [], 0
    in_sq_region = False
    for line in text.splitlines(keepends=True):
        lab = LABEL.match(line)
        if lab:
            in_sq_region = int(lab.group(2), 16) >= SQ_REGION_START
        m = SPILL.match(line.rstrip("\n"))
        if m and m.group("reg") in SAVED and not in_sq_region:
            nl = "\n" if line.endswith("\n") else ""
            out.append(
                f"{m.group('lead')}{NARROW[m.group('op')]}{m.group('gap')}"
                f"{m.group('reg')},{m.group('off')}({m.group('base')})"
                f"{m.group('tail')}{nl}"
            )
            n += 1
        else:
            out.append(line)
    return "".join(out), n


def main() -> None:
    if len(sys.argv) != 3:
        print(__doc__)
        raise SystemExit(2)
    src, dst = sys.argv[1], sys.argv[2]
    text = open(src, encoding="utf-8", errors="surrogateescape").read()
    new, n = rewrite(text)
    open(dst, "w", encoding="utf-8", errors="surrogateescape").write(new)
    print(f"fix_core_spills: narrowed {n} spill(s) {src} -> {dst}")


if __name__ == "__main__":
    main()
