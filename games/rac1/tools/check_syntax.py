#!/usr/bin/env python3
"""
Parse every C file with clang, for a MIPS target. This needs no toolchain and no disc, so CI can run it.

  python3 tools/check_syntax.py            # fail if a file has more errors than config/syntax_baseline.tsv
  python3 tools/check_syntax.py --update   # write the baseline (only do this after fixing errors)

It catches broken headers, moved declarations and bad splits. It cannot prove a match: use
tools/check_all.sh for that.

GCC 2.95 accepts some things clang does not, such as a global redeclared with another type inside a function.
The baseline lists each known error by its kind and symbol, not its line, so a new error shows even if another was
fixed. Do not fix one just to please clang: `extern char x;` and `extern char x[];` address a global differently at -G2, so the difference can be
what makes the function match.

Version fragments (src/versions/) are spliced into their shared file, so they are not parsed on their own. Files in
src/data/ are left out too: their values are generated from your baserom, and CI has none.
"""
import collections
import concurrent.futures
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "tidy"))
import clast  # noqa: E402

ROOT = clast.ROOT
BASELINE = ROOT / "config/syntax_baseline.tsv"
HEADER = ("# Errors clang finds in C that GCC 2.95 accepts (tools/check_syntax.py): file, then the error.\n"
          "# Fix one, then run --update. A new one fails the check.\n")
FLAGS = ["--target=mips64el-linux-gnuabin32", "-std=gnu89", "-fsyntax-only", "-DSKIP_ASM", "-w", "-ferror-limit=0",
         "-Wno-incompatible-pointer-types", "-Wno-int-conversion", "-Wno-implicit-function-declaration",
         "-Wno-implicit-int", "-Wno-return-type"]


def sources():
    out = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True, check=True).stdout
    return [f for f in out.split() if not f.startswith(("src/versions/", "src/libgcc/", "src/data/")) and (ROOT / f).exists()]


def errors_in(rel):
    """(file, error lines). The file is copied with same-length types (see tidy/clast.py)."""
    temp = clast.S / ("syntax_" + rel.replace("/", "_"))
    temp.write_text(clast.ee((ROOT / rel).read_text(errors="replace")))
    cmd = ["clang", *FLAGS, "-I", str(clast.INC_T), "-include", str(clast.PRELUDE), str(temp)]
    result = subprocess.run(cmd, capture_output=True, text=True)
    temp.unlink()
    return rel, [line.replace(str(temp), rel) for line in result.stderr.splitlines() if "error:" in line]


def load_baseline():
    """{file: [error key]}"""
    known = {}
    if BASELINE.exists():
        for line in BASELINE.read_text().splitlines():
            if line and not line.startswith("#"):
                name, key = line.split("\t", 1)
                known.setdefault(name, []).append(key)
    return known


def error_key(line):
    """An error without its place or types: its kind and first name. These read the same across clang versions."""
    message = line.split("error:", 1)[-1].strip()
    named = re.match(r"([^']*'[^']*')", message)
    return named.group(1) if named else message


def new_errors(found, baseline):
    """{file: [error key]} of errors the baseline does not list, counting repeats."""
    worse = {}
    for rel, errors in found.items():
        known = collections.Counter(baseline.get(rel, []))
        for key in map(error_key, errors):
            if known[key] > 0:
                known[key] -= 1
            else:
                worse.setdefault(rel, []).append(key)
    return worse


def main():
    clast.prep_includes()
    files = sources()
    with concurrent.futures.ThreadPoolExecutor() as pool:
        found = {rel: errs for rel, errs in pool.map(errors_in, files) if errs}
    if "--update" in sys.argv[1:]:
        rows = sorted((rel, error_key(error)) for rel, errors in found.items() for error in errors)
        BASELINE.write_text(HEADER + "".join(f"{rel}\t{key}\n" for rel, key in rows))
        print(f"baseline: {len(found)} files, {len(rows)} errors")
        return
    baseline = load_baseline()
    worse = new_errors(found, baseline)
    for rel, keys in sorted(worse.items()):
        print(rel)
        print("\n".join(f"  {key}" for key in keys[:5]))
    if worse:
        sys.exit(f"*** {len(worse)} file(s) have new syntax errors")
    total = sum(map(len, found.values()))
    known = sum(map(len, baseline.values()))
    note = f" ({known - total} fixed: run --update)" if known > total else ""
    print(f"{len(files)} files parsed, {total} known errors{note}")


if __name__ == "__main__":
    main()
