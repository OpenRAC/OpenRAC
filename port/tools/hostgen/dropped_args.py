#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Arguments the PS2 passed in registers that the port's C drops, ranked by how likely each matters.

    python3 port/tools/hostgen/dropped_args.py <hostgen out> [--game port/game/rac3-pal/hostgen.json]
        [--apply] [--tsv FILE]

Matched C may leave out an argument it only hands on: the register keeps the caller's value on the
EE, so the bytes match, but translated for the host the value is gone. Up Your Arsenal's
random_vector (func_L01_00443B88) names only `out` and calls rand_range() with nothing, which reads
its callers' range in $f12/$f13; crate pieces spun by a quaternion of any length. Two kinds, from
hostgen's report (asm.signature_check on):

- **definition**: a function with C whose retail code reads argument registers its definition does
  not name ("c_argument_drops", asmc.c_argument_drops). Its callers' values never reach it.
  Fixed by naming the parameters: hostgen.json "params_from_declarations" (the list) with
  "extra_params" (--apply writes both), so its own short calls hand them on (lower.py).
- **call**: a call whose C passes fewer arguments than the callee takes, where the caller has no
  argument of its own in that register ("arguments_filled_with_zero"). Harmless when the callee's
  code never reads that register (its prototype is wider than its code); fixed by the first kind
  when the caller hands on an argument it does not name; otherwise the EE's register held a value
  the caller computed for something else (a leftover), which only the decompilation's C can say.

Ranked: definitions whose dropped register a call of theirs reads (the value is lost), then floats,
then by callers; calls the callee reads before the harmless ones.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

FLOATS = {"float"}
PROTO = re.compile(r"^(?:extern\s+)?([\w\s\*]+?)\s*\b(\w+)\((.*)\);\s*$")


def protos(path: Path) -> dict[str, list[str]]:
    """name -> parameter types, from hostgen's game_protos.h."""
    out = {}
    for line in path.read_text(errors="replace").splitlines():
        m = PROTO.match(line.strip())
        if not m:
            continue
        params = m.group(3).strip()
        types = [] if params in ("", "void") else [p.strip() for p in params.split(",")]
        out[m.group(2)] = [re.sub(r"\s*\b[a-z_]\w*$", "", t) if " " in t else t for t in types]
    return out


def kind(t: str) -> str:
    return "f" if t.strip() in FLOATS else "i"


def register_of(types: list[str], k: int) -> tuple[str, int] | None:
    """The register file and index (within it) of parameter k (0-based)."""
    if k >= len(types):
        return None
    want = kind(types[k])
    return want, sum(1 for t in types[:k] if kind(t) == want)


def analyse(gen: Path) -> dict:
    report = json.loads((gen / "report.json").read_text())
    s = report.get("summary", report)
    drops = s.get("c_argument_drops", {})
    reads = dict(s.get("c_argument_reads", {}))
    sigs = protos(gen / "game_protos.h")
    for name in s.get("from_assembly", {}):
        # A function translated from its assembly takes what its code reads (asm.live_arguments).
        types = sigs.get(name)
        if types is not None:
            reads.setdefault(name, [sum(kind(t) == "i" for t in types), sum(kind(t) == "f" for t in types)])
    callers: dict[str, set] = {}
    for caller, callees in s.get("call_graph", {}).items():
        for c in callees:
            callers.setdefault(c, set()).add(caller)

    def reads_of(name: str):
        if name in reads:
            return reads[name]
        base = re.sub(r"_[0-9A-F]{8}$", "", name)   # a per-caller spelling (func_X_<caller place>)
        return reads.get(base)

    calls = []
    lost_in: dict[str, list] = {}
    for caller, sites in s.get("arguments_filled_with_zero", {}).items():
        for site in sites:
            m = re.match(r"(\S+) argument (\d+)", site)
            if not m:
                continue
            callee, k = m.group(1), int(m.group(2)) - 1
            reg = register_of(sigs.get(callee, []), k)
            r = reads_of(callee)
            if reg is None or r is None:
                verdict = "unknown"
            elif reg[1] >= r[0 if reg[0] == "i" else 1]:
                verdict = "callee does not read it"
            else:
                d = drops.get(caller)
                own = None
                if d is not None:
                    idx = 0 if reg[0] == "i" else 1
                    own = d["takes"][idx] <= reg[1] < d["reads"][idx]
                verdict = "handed on (definition)" if own else "leftover"
            regname = "?" if reg is None else (f"$f{12 + reg[1]}" if reg[0] == "f" else
                                               ["$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3"][reg[1]]
                                               if reg[1] < 8 else f"stack {reg[1] - 8}")
            row = {"caller": caller, "callee": callee, "argument": k + 1, "register": regname, "verdict": verdict}
            calls.append(row)
            if verdict != "callee does not read it":
                lost_in.setdefault(caller, []).append(row)
    definitions = []
    for name, d in drops.items():
        extra_i = max(0, d["reads"][0] - d["takes"][0])
        extra_f = max(0, d["reads"][1] - d["takes"][1])
        lost = len(lost_in.get(name, []))
        score = 100 * bool(lost) + 10 * lost + 50 * bool(extra_f) + min(len(callers.get(name, ())), 30)
        definitions.append({"function": name, "reads": d["reads"], "takes": d["takes"], "extra_int": extra_i,
                            "extra_float": extra_f, "lost_calls": lost, "callers": len(callers.get(name, ())),
                            "unit": d.get("unit"), "score": score})
    definitions.sort(key=lambda r: (-r["score"], r["function"]))
    order = {"leftover": 0, "handed on (definition)": 1, "unknown": 2, "callee does not read it": 3}
    calls.sort(key=lambda r: (order[r["verdict"]], not r["register"].startswith("$f"), r["caller"], r["argument"]))
    return {"definitions": definitions, "calls": calls}


def host_called(game: Path, cfg: dict) -> set[str]:
    text = ""
    for d in [game.parent / cfg.get("host_dir", "host"), *[game.parent / x for x in cfg.get("host_shared", [])],
              game.parent.parent / str(cfg.get("game", "")) / "host"]:
        if d.is_dir():
            text += "".join(p.read_text(errors="replace") for p in sorted(d.glob("*.c")))
    return set(re.findall(r"\b(?:func|FUN)_\w+", text))


def apply(game: Path, definitions: list[dict]) -> list[str]:
    """Adds each definition's missing parameters to hostgen.json: "params_from_declarations" (the
    list) and "extra_params" (int for an integer register, float for a float one). Not the
    functions the port's own C calls (it calls them by the prototype it has)."""
    cfg = json.loads(game.read_text())
    listed = cfg.get("params_from_declarations")
    if not isinstance(listed, list):
        raise SystemExit("dropped_args: --apply needs hostgen.json \"params_from_declarations\" as a list")
    extra = cfg.setdefault("extra_params", {})
    skip = host_called(game, cfg)
    added = []
    for d in definitions:
        name = d["function"]
        if name in skip:
            continue
        types = list(extra.get(name, []))
        have_i = sum(1 for t in types if t != "float")
        have_f = len(types) - have_i
        types += ["int"] * max(0, d["extra_int"] - have_i) + ["float"] * max(0, d["extra_float"] - have_f)
        if types != extra.get(name):
            extra[name] = types
            added.append(name)
        if name not in listed:
            listed.append(name)
    cfg["extra_params"] = dict(sorted(extra.items()))
    cfg["params_from_declarations"] = sorted(set(listed))
    game.write_text(json.dumps(cfg, indent=2, ensure_ascii=False) + "\n")
    return added


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("gen", type=Path, help="hostgen's --out (report.json, game_protos.h)")
    ap.add_argument("--game", type=Path, help="the game's hostgen.json, for --apply")
    ap.add_argument("--apply", action="store_true", help="name the definitions' missing parameters in --game")
    ap.add_argument("--tsv", type=Path, help="write the ranked list here")
    args = ap.parse_args(argv)
    found = analyse(args.gen)
    defs, calls = found["definitions"], found["calls"]
    lines = ["# kind\tscore\tfunction\treads (int float)\ttakes\tlost calls\tcallers\tunit"]
    for d in defs:
        lines.append(f"definition\t{d['score']}\t{d['function']}\t{d['reads'][0]} {d['reads'][1]}\t"
                     f"{d['takes'][0]} {d['takes'][1]}\t{d['lost_calls']}\t{d['callers']}\t{d['unit']}")
    lines.append("# kind\tverdict\tcaller\tcallee\targument\tregister")
    for c in calls:
        lines.append(f"call\t{c['verdict']}\t{c['caller']}\t{c['callee']}\t{c['argument']}\t{c['register']}")
    text = "\n".join(lines) + "\n"
    if args.tsv:
        args.tsv.write_text(text)
    by = {}
    for c in calls:
        by[c["verdict"]] = by.get(c["verdict"], 0) + 1
    print(f"dropped_args: {len(defs)} definitions name fewer arguments than their code reads "
          f"({sum(1 for d in defs if d['extra_float'])} of them floats); {len(calls)} zero-filled call arguments: "
          + ", ".join(f"{v} {k}" for k, v in sorted(by.items())))
    if args.apply:
        if args.game is None:
            ap.error("--apply needs --game")
        added = apply(args.game, defs)
        print(f"dropped_args: {len(added)} functions given their parameters in {args.game}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
