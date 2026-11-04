#!/usr/bin/env python3
"""
Give a local hero pointer its real type.

  python3 tools/tidy/hero_pointers.py            # dry run: show counts
  python3 tools/tidy/hero_pointers.py --write [FILE...]

`char *p = g_hero;` used only as `((struct Hero *)p)->x` becomes `struct Hero *p = (struct Hero *)g_hero;` and `p->x`.
A pointer used any other way stays, because `p + 8` means something else on a struct pointer. --write bisects each file
with fake_bases.check, and a declaration that stops matching stays as it was. Run tools/format.py and tools/gate_loop.py
afterwards.
"""
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fake_bases as fb  # noqa: E402
import prune_externs as pe  # noqa: E402
import voice_rows as vr  # noqa: E402

ROOT = pe.ROOT
DECLARATION = re.compile(r"\b(?:unsigned\s+)?char\s*\*\s*(\w+)\s*=\s*(\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?(&?g_hero)\s*;")


def cast_use(name):
    return re.compile(rf"\(\s*\(\s*struct\s+Hero\s*\*\s*\)\s*{name}\s*\)(?=\s*->)|\(\s*struct\s+Hero\s*\*\s*\)\s*{name}\b")


def sites(text):
    """[(declaration start, end of its block, name, how many casts)] that can convert."""
    code = pe.COMMENT_OR_STRING.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)
    found = []
    for m in DECLARATION.finditer(code):
        name = m.group(1)
        end = vr.block_end(code, m.end())
        scope = code[m.end():end]
        words = len(re.findall(rf"\b{name}\b", scope))
        casts = len(cast_use(name).findall(scope))
        if casts and casts == words:
            found.append((m.start(), end, name, casts))
    return found


def apply(text, chosen):
    """Convert the sites at these declaration starts, last first so offsets stay right."""
    for start, end, name, _ in sorted(chosen, reverse=True):
        m = DECLARATION.match(text, start)
        target = m.group(3)
        new = f"struct Hero *{name} = (struct Hero *){target};"
        scope = cast_use(name).sub(name, text[m.end():end])
        text = text[:start] + new + scope + text[end:]
    return text


def land(rel):
    path = ROOT / rel
    original = path.read_text(errors="surrogateescape")
    found = sites(original)
    if not found:
        return 0, 0, 0

    def passes(group):
        path.write_text(apply(original, group), errors="surrogateescape")
        bad, _, failed = fb.check(rel)
        return not bad and not failed
    chosen = fb.accepted_functions(found, passes)
    path.write_text(apply(original, chosen), errors="surrogateescape")
    return len(chosen), len(found), sum(site[3] for site in chosen)


def main():
    args = sys.argv[1:]
    write = "--write" in args
    paths = [a for a in args if not a.startswith("--")]
    if not paths:
        paths = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout.split()
    total = declarations = kept = 0
    for rel in paths:
        if rel.startswith(("src/versions/", "src/data/")):
            continue
        if write:
            done, tried, casts = land(rel)
            if tried:
                print(f"{rel}: {done}/{tried}", flush=True)
            declarations += done
            kept += tried - done
            total += casts
        else:
            found = sites((ROOT / rel).read_text(errors="surrogateescape"))
            declarations += len(found)
            total += sum(site[3] for site in found)
    print(f"{'converted' if write else 'would convert'} {declarations} pointers, {total} casts; {kept} stay")


if __name__ == "__main__":
    main()
