#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Materialize only explicit SDATA(symbol) address aliases before assembly.

Adapted from the declared-SDATA part of RAC1's tools/check_macro_slots.py,
revision209d3381ebaa8c06d5df98eeabad4adad45bdfb7 (GPL-3.0, repository LICENSE).
The existing common.h macro reserves the assembler spelling symbol__gp.
Unlike RAC1's broader pass, this never changes instructions or operands and
never discovers ordinary macro accesses in branch delay slots.

The alias's .extern size1 selects the assembler's small-address form; it is
address-class metadata, not a C object bound. The canonical owner's declaration
and ELF extent stay unchanged. Its alias is equated to that same storage.
Incomplete C arrays emit no .extern, so explicit suffix references are recognized
outside strings/comments. Metadata precedes the first .ent (after the compiler
preamble); the binding is appended, preserving GNU as function-size accounting.
"""
import re
import sys
from pathlib import Path

SYMBOL = r"[A-Za-z_.$][\w.$]*"
ALIASES = re.compile(r"(?<![\w.$])" + SYMBOL + r"__gp(?![\w.$])")
MASK = re.compile(r'"(?:\\.|[^"\\])*"|\x27(?:\\.|[^\x27\\])*\x27|/\*[\s\S]*?\*/|#[^\n]*')
EXTERN = re.compile(r"(?m)^([ \t]*\.extern[ \t]+)(" + SYMBOL
                    + r")([ \t]*,[ \t]*)([0-9]+)([ \t]*)(?=\n|$)")


def masked(text: str) -> str:
    """Keep source positions/newlines while hiding strings and assembler comments."""
    return MASK.sub(lambda m: re.sub(r"[^\n]", " ", m[0]), text)


def transform(text: str) -> str:
    """Return address-alias metadata, or the exact original text without SDATA."""
    clean = masked(text)
    aliases = {m[0]: m[0][:-4] for m in ALIASES.finditer(clean)}
    if not aliases:
        return text
    bound = set()
    for alias, owner in aliases.items():
        if owner.endswith("__gp"):
            raise ValueError(f"nested SDATA alias: {alias}")
        definition = (r"(?m)^[ \t]*(?:" + re.escape(alias)
                      + r"[ \t]*:|\.(?:comm|lcomm|common)[ \t]+"
                      + re.escape(alias) + r"(?=[, \t]))")
        if re.search(definition, clean):
            raise ValueError(f"SDATA alias defines storage: {alias}")
        binding = (r"(?m)^[ \t]*(?:" + re.escape(alias)
                   + r"[ \t]*=[ \t]*|\.(?:equ|equiv|set)[ \t]+"
                   + re.escape(alias) + r"[ \t]*,[ \t]*)([^\n]*)$")
        for match in re.finditer(binding, clean):
            if match[1].strip() != owner:
                raise ValueError(f"conflicting SDATA binding: {alias}")
            bound.add(alias)
        declarations = r"(?m)^[ \t]*\.extern[ \t]+" + re.escape(alias) + r"([^\n]*)$"
        for match in re.finditer(declarations, clean):
            if not re.fullmatch(r"[ \t]*,[ \t]*[0-9]+[ \t]*", match[1]):
                raise ValueError(f"invalid SDATA extern: {alias}")
    # Only explicit aliases' numeric metadata changes, never canonical declarations.
    edits = [(m.start(4), m.end(4)) for m in EXTERN.finditer(clean) if m[2] in aliases]
    for start, end in reversed(edits):
        text = text[:start] + "1" + text[end:]
    clean = masked(text)
    entry = re.search(r"(?m)^[ \t]*\.ent\b", clean)
    if entry:
        at = entry.start()
    else:
        instruction = re.search(r"(?m)^[ \t]*[A-Za-z][\w.]*[ \t]+\$", clean)
        at = instruction.start() if instruction else len(text)
    early = {m[2] for m in EXTERN.finditer(clean[:at])
             if m[2] in aliases and m[4] == "1"}
    head = "".join(f"\t.extern {a}, 1\n" for a in sorted(aliases.keys() - early))
    text = text[:at] + head + text[at:]
    tail = "".join(f"\t{a} = {aliases[a]}\n" for a in sorted(aliases.keys() - bound))
    if tail:
        text += ("" if text.endswith("\n") else "\n") + tail
    return text


def main(path: str) -> int:
    source = Path(path)
    text = source.read_text()
    try:
        result = transform(text)
    except ValueError as error:
        print(f"sdata_aliases: {source}: {error}", file=sys.stderr)
        return 1
    if result != text:
        source.write_text(result, newline="\n")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: sdata_aliases.py file.s")
    sys.exit(main(sys.argv[1]))