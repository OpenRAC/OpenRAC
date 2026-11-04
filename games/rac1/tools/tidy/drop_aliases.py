#!/usr/bin/env python3
"""
Remove local aliases that do nothing.

  python3 tools/tidy/drop_aliases.py src/game           # dry run: show counts
  python3 tools/tidy/drop_aliases.py src/game --write
  python3 tools/tidy/drop_aliases.py src/game --list

`extern s32 level_r[] __asm__("D_001313D0");` is the same symbol as `level`. When it is the file's only view of that
symbol, it becomes `extern s32 D_001313D0[];`, and the code says `level` (include/names/ maps the name to the symbol).

An alias stays when:
  - the symbol or its readable name appears anywhere else in the file or its version fragments: the file has two views
  - a header declares the symbol: the types could clash
  - a version fragment uses the alias
  - it has more than one name, or uses a $gp label (SDATA)
  - the symbol has no readable name: then the alias is the only name a reader gets
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import merge_types as mt  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
ALIAS = re.compile(r'\s*__asm__\s*\(\s*"((?:func|D)_(?:L\d+_)?[0-9A-Fa-f]+)"\s*\)')


def header_words():
    words = set()
    for path in (ROOT / "include").rglob("*.h"):
        if path.parent.name != "names" and path.name != "names.h":
            words |= set(pe.WORD.findall(pe.without_comments(path.read_text(errors="replace"))))
    return words


def readable(symbol, aliases):
    names = [n for n in aliases.get(symbol, ()) if not n.startswith(("func_", "D_"))]
    return names[0] if len(names) == 1 else None


def plan(text, fragments, aliases, in_headers):
    """[(start, end, new statement, alias, new name)] for the aliases this file can drop."""
    edits = []
    statements = pe.find_externs(text)
    for start, end, statement in statements:
        m = ALIAS.search(statement)
        names = pe.declared_names(statement)
        if not m or not names or len(names) != 1:
            continue
        alias, symbol = names[0], m.group(1)
        name = readable(symbol, aliases)
        if not name:
            continue                        # the alias is the only readable name this symbol has
        others = {symbol, name}
        if alias in others or others & in_headers:
            continue
        rest = pe.without_comments(text[:start] + text[end:])
        words = set(pe.WORD.findall(rest))
        if others & words or others & set(pe.WORD.findall(pe.without_comments(fragments))):
            continue
        if re.search(rf"\b{re.escape(alias)}\b", pe.without_comments(fragments)):
            continue
        if statement.count(alias) != 1:
            continue
        new = ALIAS.sub("", statement).replace(alias, symbol)
        edits.append((start, end, new, alias, name))
    return edits


def apply_plan(text, edits):
    out, last = [], 0
    for start, end, new, _, _ in edits:
        out.append(text[last:start])
        out.append(new)
        last = end
    out.append(text[last:])
    return mt.rename_code("".join(out), {alias: name for _, _, _, alias, name in edits})


def main():
    args = sys.argv[1:]
    write, listing = "--write" in args, "--list" in args
    paths = [a for a in args if not a.startswith("--")]
    if not paths:
        sys.exit(__doc__)
    aliases, in_headers = pe.load_aliases(), header_words()
    total = files = 0
    for rel in pe.sources(paths):
        if rel.startswith("src/data/"):
            continue
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        edits = plan(text, pe.fragments_text(rel), aliases, in_headers)
        if not edits:
            continue
        total += len(edits)
        files += 1
        if listing:
            for _, _, new, alias, name in edits:
                print(f"{rel}: {alias} -> {name}: {' '.join(new.split())[:70]}")
        if write:
            path.write_text(apply_plan(text, edits), errors="surrogateescape")
    print(f"{'removed' if write else 'would remove'} {total} aliases in {files} files")


if __name__ == "__main__":
    main()
