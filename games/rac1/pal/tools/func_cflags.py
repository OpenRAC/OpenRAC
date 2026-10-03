#!/usr/bin/env python3
"""
Per-function compiler flags: splice functions compiled with extra flags
into a file's compiled assembly.

  python tools/func_cflags.py SRC.c OUT.s -- CC [CFLAGS...]

Retail's game code was not built with one flag set per source file: some
functions use -mno-split-addresses (every global reached through the
assembler's `lui $at` macro, never the compiler's own %hi/%lo split),
mixed within a file with split-address neighbours, as
ratchet-uya-decomp found for R&C 3. config/func_cflags.txt lists those
functions with their extra flags.

For each listed function defined in SRC.c, this compiles SRC.c again
with the extra flags and replaces the function's `.ent`..`.end` block in
OUT.s (the normal compile) with the one from that compile. Local labels
in the spliced block get a unique suffix so they cannot collide.
Functions whose block would need rodata from the other compile (switch
jump tables) are refused.

Run by Makefile.sn and tools/try_func.py right after `-S`; a file with no
listed function is left untouched.
"""
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CONFIG = REPO / "config" / "func_cflags.txt"


def load_config():
    out = {}
    if not CONFIG.exists():
        return out
    for line in CONFIG.read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        name, *flags = line.split()
        out[name] = tuple(flags)
    return out


def block(lines, name):
    """(first, last) line indices of NAME's .ent..end block, or None."""
    ent = re.compile(rf"^\s*\.ent\s+{name}\s*$")
    end = re.compile(rf"^\s*\.end\s+{name}\s*$")
    first = next((i for i, l in enumerate(lines) if ent.match(l)), None)
    if first is None:
        return None
    last = next(i for i in range(first, len(lines)) if end.match(lines[i]))
    return first, last


def main():
    if "--" not in sys.argv:
        sys.exit(__doc__)
    sep = sys.argv.index("--")
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    cc = sys.argv[sep + 1:]

    wanted = load_config()
    lines = out.read_text().splitlines()
    todo = {n: f for n, f in wanted.items() if block(lines, n)}
    if not todo:
        return

    by_flags = {}
    for name, flags in todo.items():
        by_flags.setdefault(flags, []).append(name)

    for k, (flags, names) in enumerate(sorted(by_flags.items())):
        tmp = out.with_name(out.stem + f"_ff{k}.s")
        cmd = cc + list(flags) + ["-S", "-o", str(tmp), str(src)]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode:
            sys.stderr.write(r.stdout + r.stderr)
            sys.exit(f"func_cflags: compiling {src} with {' '.join(flags)} failed")
        other = tmp.read_text().splitlines()
        for name in names:
            a, b = block(other, name)
            body = other[a:b + 1]
            if any(re.search(r"\.(rdata|section|word\s+\$L)", l) for l in body):
                sys.exit(f"func_cflags: {name} has rodata (a jump table?); not supported")
            body = [re.sub(r"\$L(\d+)", rf"$L\1_ff{k}", l) for l in body]
            a2, b2 = block(lines, name)
            lines[a2:b2 + 1] = body
        tmp.unlink()
    out.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
