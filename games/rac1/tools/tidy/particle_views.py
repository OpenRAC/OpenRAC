#!/usr/bin/env python3
"""
Give particle fields to byte pointers that hold a new particle.

  python3 tools/tidy/particle_views.py [--write] [--list] [FILE...]

A local is a particle only when its every assignment is a call of a create_part function, which is what makes the type
certain. The file gets include/structs/particle.h, and tools/tidy/rawstruct.py does the conversion. Data at 0x20 and up
is each type's own, so it stays raw. Run tools/gate_loop.py after --write.

--skip=A,B leaves those functions raw, by address name or readable name: use it for functions the gate says change.
"""
import collections
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import moby_views as mv  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = mv.ROOT
SPAWN = r"\s*(?:\([^()]*\)\s*)?(?:create_part\d*|func_00218928)\s*\("
HEADER = '#include "structs/particle.h"\n'
SHARED_FIELDS = 0x20


def is_particle(body, var):
    """The base-record offsets used through var, if it only ever holds a new particle."""
    assigned = re.findall(rf"\b{var}\s*=(?!=)", body)
    spawned = re.findall(rf"\b{var}\s*={SPAWN}", body)
    if not spawned or len(assigned) != len(spawned):
        return None
    offsets = {int(m.group(3), 0) for m in re.finditer(mv.RAW.format(v=var), body)}
    return {o for o in offsets if o < SHARED_FIELDS} or None


def candidates(text):
    """{var: [function]}"""
    found = collections.defaultdict(list)
    clean = mv.COMMENT.sub(" ", text)
    starts = list(mv.FUNC.finditer(clean))
    for i, m in enumerate(starts):
        end = starts[i + 1].start() if i + 1 < len(starts) else len(clean)
        body = clean[m.end():end]
        names = set(re.findall(rf"\b(\w+)\s*={SPAWN}", body))
        for var in sorted(names):
            if mv.declared_as_bytes(m.group(2), body, var) and is_particle(body, var):
                found[var].append(m.group(1))
    return found


def add_header(text):
    if HEADER in text:
        return text
    m = re.search(r'^#include "common.h"\n', text, re.M)
    return text[:m.end()] + HEADER + text[m.end():] if m else None


def main():
    args = sys.argv[1:]
    write, listing = "--write" in args, "--list" in args
    paths = [a for a in args if not a.startswith("--")]
    skip = set(",".join(a[7:] for a in args if a.startswith("--skip=")).split(",")) - {""}
    for name, others in pe.load_aliases().items():
        if name in skip:
            skip |= set(others)
    if not paths:
        out = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout
        paths = [p for p in out.split() if not p.startswith(("src/data/", "src/libgcc/", "src/versions/"))]
    total = 0
    for rel in paths:
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        found = {var: [f for f in funcs if f not in skip] for var, funcs in candidates(text).items()}
        found = {var: funcs for var, funcs in found.items() if funcs}
        for var, funcs in found.items():
            total += len(funcs)
            if listing:
                print(f"{rel}: {var}: {','.join(funcs)}"[:140])
        if not (write and found):
            continue
        with_header = add_header(text)
        if with_header is None:
            print(f"{rel}: no common.h include, skipped")
            continue
        path.write_text(with_header, errors="surrogateescape")
        for var, funcs in found.items():
            run = subprocess.run([sys.executable, str(mv.HERE / "rawstruct.py"), rel, f"--func={','.join(funcs)}",
                                  f"--var={var}", "--struct=struct PartBase"], cwd=ROOT, capture_output=True, text=True)
            if run.returncode:
                print(f"{rel}: {var}: rawstruct stopped: {run.stderr.strip()[-100:]}")
    print(f"{'converted' if write else 'would convert'} {total} variables")


if __name__ == "__main__":
    main()
