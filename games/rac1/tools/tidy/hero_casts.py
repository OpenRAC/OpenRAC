#!/usr/bin/env python3
"""
Read the hero directly, not through a cast of its own address.

  python3 tools/tidy/hero_casts.py            # dry run: show counts
  python3 tools/tidy/hero_casts.py --write [FILE...]

`((Hero *)&g_hero)->state` becomes `g_hero.state`, in a file that declares `extern Hero g_hero;`. --write bisects each
file by function with fake_bases.check, and a function that stops matching keeps its spelling. Run tools/format.py and
tools/gate_loop.py afterwards.
"""
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fake_bases as fb  # noqa: E402
import moby_views as mv  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
DECLARED = re.compile(r"^\s*extern\s+(?:struct\s+)?Hero\s+g_hero\s*;", re.M)
SITE = re.compile(r"\(\s*\(\s*(?:struct\s+)?Hero\s*\*\s*\)\s*&\s*g_hero\s*\)\s*->\s*")


def spans(text):
    """[(function name, start, end)]: each runs to the next function, and the first from the top of the file."""
    masked = pe.COMMENT_OR_STRING.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)
    heads = [(m.group(1), m.start()) for m in mv.FUNC.finditer(masked)]
    return [(name, at, heads[i + 1][1] if i + 1 < len(heads) else len(text)) for i, (name, at) in enumerate(heads)]


def owners(text):
    """{function name: sites in it}, for a file that can convert."""
    if not DECLARED.search(pe.without_comments(text)):
        return {}
    found = {}
    for name, start, end in spans(text):
        count = len(SITE.findall(text[start:end]))
        if count:
            found[name] = count
    return found


def apply(text, chosen):
    for name, start, end in reversed(spans(text)):
        if name in chosen:
            text = text[:start] + SITE.sub("g_hero.", text[start:end]) + text[end:]
    return text


def land(rel):
    path = ROOT / rel
    original = path.read_text(errors="surrogateescape")
    found = owners(original)
    if not found:
        return 0, 0

    def passes(group):
        path.write_text(apply(original, set(group)), errors="surrogateescape")
        bad, _, failed = fb.check(rel)
        return not bad and not failed
    chosen = fb.accepted_functions(set(found), passes)
    path.write_text(apply(original, chosen), errors="surrogateescape")
    print(f"{rel}: {len(chosen)}/{len(found)} functions", flush=True)
    return sum(found[name] for name in chosen), sum(found.values())


def main():
    args = sys.argv[1:]
    write = "--write" in args
    paths = [a for a in args if not a.startswith("--")]
    if not paths:
        paths = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout.split()
    done = total = 0
    for rel in paths:
        if rel.startswith(("src/versions/", "src/data/")):
            continue
        if write:
            a, b = land(rel)
            done, total = done + a, total + b
        else:
            count = sum(owners((ROOT / rel).read_text(errors="surrogateescape")).values())
            done, total = done + count, total + count
    print(f"{'converted' if write else 'would convert'} {done} of {total} sites")


if __name__ == "__main__":
    main()
