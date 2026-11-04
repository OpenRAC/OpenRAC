#!/usr/bin/env python3
"""
Use the real name where a local function alias has the same type as everyone else's declaration.

  python3 tools/tidy/func_aliases.py            # dry run: show counts
  python3 tools/tidy/func_aliases.py --write [FILE...]
  python3 tools/tidy/func_aliases.py --header asm_functions/executable.h [--write]    # only that header's symbols
  python3 tools/tidy/func_aliases.py --header none [--write]                          # symbols no header declares

`extern void vsub_2d8e48(void *, void *, void *) __asm__("func_001F9BF0");` and its calls become `FastVecSub`. Only an
alias with the types of the symbol's header prototype, or of most files if it has none: an alias whose types really
differ is there for a reason, because argument order and scalar width change the code. Pointer types do not count.
--write bisects each file with fake_bases.check, and an alias that stops matching stays. Run tools/tidy/move_externs.py
afterwards to take the plain declarations into headers, then tools/format.py and tools/gate_loop.py.
"""
import collections
import concurrent.futures
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fake_bases as fb  # noqa: E402
import merge_types as mt  # noqa: E402
import move_externs as me  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
LABEL = re.compile(r'\s*__asm__\s*\(\s*"((?:func)_(?:L\d+_)?[0-9A-Fa-f]+)"\s*\)')


def alias_lines(text):
    """[(start, end, alias name, label, prototype)] of each function alias at file scope."""
    found = []
    for start, end, statement in me.declarations(text):
        if not me.is_file_scope(text, start):
            continue
        m = LABEL.search(statement)
        if not m:
            continue
        plain = LABEL.sub("", statement)
        if me.QUALIFIED.search(plain):
            continue
        names = pe.declared_names(plain) or []
        if len(names) != 1 or not re.search(rf"\b{re.escape(names[0])}\s*\(", plain):
            continue
        prototype = me.split_prototype(names[0], plain)
        if prototype:
            found.append((start, end, names[0], m.group(1), prototype))
    return found


def wanted_shapes(sources, texts):
    """{symbol: the shape a plain declaration of it should have}"""
    shapes = collections.defaultdict(collections.Counter)
    for symbol, rows in sources.lines.items():
        shapes[symbol].update(me.shape(row[2]) for row in rows)
    for text in texts.values():
        for _, _, _, label, prototype in alias_lines(text):
            shapes[sources.symbol(label)][me.shape(prototype)] += 1
    wanted = {}
    for symbol, counts in shapes.items():
        counts.pop(None, None)
        if symbol in sources.existing:
            name = pe.declared_names(sources.existing[symbol][1])[0]
            existing = me.split_prototype(name, sources.existing[symbol][1])
            if existing and me.shape(existing) is not None:
                wanted[symbol] = me.shape(existing)
            continue
        if counts:
            wanted[symbol] = sorted(counts.items(), key=lambda row: (-row[1], str(row[0])))[0][0]
    return wanted


def real_name(sources, label):
    names = {label} | sources.aliases.get(label, set())
    readable = [n for n in names if not re.match(r"(?:func|D)_", n)]
    return readable[0] if len(readable) == 1 else label


def matching(text, sources, wanted, rel):
    """{alias: (label, real name)} for the aliases in this file that can go."""
    found, odd = {}, set()
    for _, _, alias, label, prototype in alias_lines(text):
        symbol = sources.symbol(label)
        if rel in sources.defined.get(symbol, ()) or symbol in sources.blocked:
            odd.add(alias)
        elif symbol in wanted and me.shape(prototype) == wanted[symbol]:
            found[alias] = (label, real_name(sources, label))
        else:
            odd.add(alias)
    return {alias: row for alias, row in found.items() if alias not in odd}


def apply(text, chosen):
    """Rename the chosen aliases, and make their declarations plain."""
    for start, end, alias, _, prototype in reversed(alias_lines(text)):
        if alias in chosen:
            result, params = prototype
            space = "" if result.endswith("*") else " "
            text = text[:start] + f"extern {result}{space}{alias}({', '.join(params)});" + text[end:]
    return mt.rename_code(text, {alias: chosen[alias][1] for alias in chosen})


def land(rel, sources, wanted):
    path = ROOT / rel
    original = path.read_text(errors="surrogateescape")
    found = matching(original, sources, wanted, rel)
    if not found:
        return 0, 0

    def passes(group):
        path.write_text(apply(original, {a: found[a] for a in group}), errors="surrogateescape")
        bad, _, failed = fb.check(rel)
        return not bad and not failed
    chosen = {a: found[a] for a in fb.accepted_functions(set(found), passes)}
    path.write_text(apply(original, chosen), errors="surrogateescape")
    renames = {alias: row[1] for alias, row in chosen.items()}
    for fragment in fb.fragments_of(rel):
        old = fragment.read_text(errors="surrogateescape")
        new = mt.rename_code(old, renames) if renames else old
        if new != old:
            fragment.write_text(new, errors="surrogateescape")
    return len(chosen), len(found)


def main():
    args = sys.argv[1:]
    write = "--write" in args
    header = args[args.index("--header") + 1] if "--header" in args else None
    paths = [a for i, a in enumerate(args) if not a.startswith("--") and (not i or args[i - 1] != "--header")]
    sources = me.Sources()
    wanted = wanted_shapes(sources, sources.text)
    if header:
        home = None if header == "none" else header
        wanted = {s: v for s, v in wanted.items() if sources.existing.get(s, (None,))[0] == home}
    paths = paths or sources.files
    done = total = lines = 0
    if write:
        with concurrent.futures.ThreadPoolExecutor(me.WORKERS) as pool:
            for rel, (a, b) in zip(paths, pool.map(lambda rel: land(rel, sources, wanted), paths)):
                if b:
                    print(f"{rel}: {a}/{b}", flush=True)
                done, total = done + a, total + b
    else:
        for rel in paths:
            count = len(matching(sources.text[rel], sources, wanted, rel))
            lines += len(alias_lines(sources.text[rel]))
            done, total = done + count, total + count
    print(f"{'converted' if write else 'would convert'} {done} of {total} aliases; {lines or ''} alias lines seen".rstrip())


if __name__ == "__main__":
    main()
