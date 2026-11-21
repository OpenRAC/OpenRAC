# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""asm.signature_check over the calls a folded function's copies make to different functions.

A folded name (one catalogue row, its copies at several places) calls, at one of its jals, a
different function in different levels: Up Your Arsenal's 28-byte wrapper func_L01_003B2DC8 calls
func_L01_00459AF8 in Veldin but a class update in Florana. hostgen reaches such a call through
openrac_call_in_copy with the caller's own registers, so what the caller hands on is what its
signature takes: an argument register any copy's callee reads that the caller's code never writes
before the call, and that its signature leaves out, reaches that callee as 0 (or, through a C call,
as whatever the host held: moby_remove was handed a host code address). The table
(variant_callees.py, hostgen.json asm "variant_callees") lists each such call's callees and what
the caller's code passes on; this compares their signatures with the caller's:

- a callee reading an argument register the caller passes on but does not take;
- a call whose result is the caller's, where the copies' callees hand back different kinds (a float
  in some levels, an integer in others): no one signature serves both, its places are split
  (tables/split_places.tsv).

The findings join asm.signature_check's (asmc.Translator.dropped), with the same allow list.
"""
from __future__ import annotations

from collections import defaultdict
from pathlib import Path

from program import canon

FLOAT = {"float"}
VOID = {"void"}


def read(path: Path) -> list[tuple[str, str, int, str, int, set[str], set[str]]]:
    out = []
    for line in path.read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 7:
            continue
        out.append((cols[0], cols[1], int(cols[2]), cols[3], int(cols[4]),
                    set(cols[5].split()) - {"-"}, set(cols[6].split()) - {"-"}))
    return out


def _counts(sig, typedefs) -> tuple[int, int, str]:
    """A signature's integer and float parameters and its result kind: float, void or int."""
    kinds = [canon(p, typedefs) for p in sig.params]
    floats = sum(1 for k in kinds if k in FLOAT)
    ret = canon(sig.ret, typedefs)
    return len(kinds) - floats, floats, "float" if ret in FLOAT else "void" if ret in VOID else "int"


def findings(program, rows: list) -> dict[str, list[str]]:
    typedefs = program.typedefs
    calls: dict[tuple[str, int], list] = defaultdict(list)
    for row in rows:
        calls[(row[0], row[4])].append(row)
    out: dict[str, list[str]] = defaultdict(list)
    for (caller, word), group in sorted(calls.items()):
        f = program.functions.get(caller)
        if f is None:
            continue
        have_i, have_f, ret = _counts(f.sig, typedefs)
        have_i += sum(1 for p in f.extra if canon(p, typedefs) not in FLOAT)
        passes, result = group[0][5], group[0][6]
        # A call that sets any of its first four arguments of a kind itself is not handing that kind
        # on: a register it leaves past them is the EE's leftover, as in every copy (func_00391130
        # sets $a0-$a3 and $t0 for a callee declared with six).
        ints_on = all(f"a{k}" in passes for k in range(4))
        floats_on = all(f"f{12 + k}" in passes for k in range(4))
        need_i = need_f = 0
        why_i = why_f = ""
        kinds: dict[str, str] = {}
        for _, callee, level, place, _, _, _ in group:
            g = program.functions.get(callee)
            if g is None:
                continue
            ints, floats, kind = _counts(g.sig, typedefs)
            for k in range(min(ints, 8) if ints_on else 0):
                if f"a{k}" in passes and k + 1 > need_i:
                    need_i, why_i = k + 1, f"{callee} (level {level}, {place})"
            for k in range(min(floats, 8) if floats_on else 0):
                if f"f{12 + k}" in passes and k + 1 > need_f:
                    need_f, why_f = k + 1, f"{callee} (level {level}, {place})"
            if kind != "void":
                kinds.setdefault(kind, f"{callee} (level {level})")
        if need_i > have_i:
            out[caller].append(f"its call at word {word} hands {need_i} integer argument registers on to "
                               f"{why_i} in another copy, its signature takes {have_i}")
        if need_f > have_f:
            out[caller].append(f"its call at word {word} hands {need_f} float argument registers on to "
                               f"{why_f} in another copy, its signature takes {have_f}")
        if result and len(kinds) > 1:
            out[caller].append(f"its call at word {word} hands back the result of {kinds['float']}, a float, "
                               f"and of {kinds['int']}, an integer, in other copies: split its places "
                               "(tables/split_places.tsv)")
    return dict(out)
