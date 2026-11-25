#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""
Repeated code in OpenRAC's own C and C++ (docs/CODING_CONVENTIONS.md, section 22.6).

  python3 tools/dup_check.py                 list copies not in the baseline; exit 1 if any
  python3 tools/dup_check.py --all           list every copy, the baseline's too
  python3 tools/dup_check.py --report-only   the check, but always exit 0 (CI uses it for now)
  python3 tools/dup_check.py --write-baseline
                                             write every copy found to the baseline
  python3 tools/dup_check.py --tokens N      the shortest run that counts (default 60)
  python3 tools/dup_check.py PATH...         only the files under these paths

A copy is a run of at least --tokens tokens that appears twice, compared after comments are
removed and every number and string literal is replaced by one placeholder. Names are compared
as written, so two loops over different fields are not a copy, but the same function pasted into
two games' folders is.

Scope: the .c, .cc, .cpp, .h, .hpp and .mm files git tracks under port/ and sdk/, without
generated files (a "Do not edit" in their first two lines) and without tests (a tests/ folder or a
*_test file), whose cases look alike on purpose.

The baseline (tools/dup_check_baseline.tsv) holds one line per accepted copy: the two files and a
hash of the repeated tokens, so moving the code within its files does not change the line. A line
may end in a tab and the reason the copy stays.
"""
import argparse
import hashlib
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Set, Tuple

ROOT = Path(__file__).resolve().parent.parent
TREES = ("port", "sdk")
SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".mm"}
BASELINE = ROOT / "tools" / "dup_check_baseline.tsv"
DEFAULT_TOKENS = 60
# A window shared by more places than this is boilerplate (an include block, a table header),
# not a copy anyone would merge; skipping it keeps the run time linear.
BUCKET_LIMIT = 8

TOKEN = re.compile(
    r"""
    (?P<comment>//[^\n]*|/\*.*?\*/)
  | (?P<string>"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*')
  | (?P<number>\.?\d[\w.']*)
  | (?P<name>[A-Za-z_]\w*)
  | (?P<space>\s+)
  | (?P<punct>::|->|<<=|>>=|<<|>>|<=|>=|==|!=|&&|\|\||\+\+|--|[-+*/%&|^!~=<>?:;,.(){}\[\]#])
  | (?P<other>.)
    """,
    re.VERBOSE | re.DOTALL,
)

Token = Tuple[str, int]


def is_generated(text: str) -> bool:
    """A generated file says so in its first two lines."""
    return "Do not edit" in "\n".join(text.splitlines()[:2])


def is_test(path: str) -> bool:
    """Tests repeat their set-up on purpose; they are not checked."""
    parts = path.split("/")
    return "tests" in parts or Path(path).stem.endswith("_test")


def tracked_sources(paths: Sequence[str]) -> List[str]:
    """The files in scope, as paths relative to the repository."""
    roots = list(paths) or list(TREES)
    listed = subprocess.run(["git", "ls-files", "--", *roots], cwd=ROOT, check=True,
                            capture_output=True, text=True).stdout.split("\n")
    return sorted(p for p in listed
                  if p and p.split("/")[0] in TREES and Path(p).suffix in SUFFIXES and not is_test(p))


def tokens_of(text: str) -> List[Token]:
    """The file's tokens with their line numbers; literals become placeholders."""
    out: List[Token] = []
    line = 1

    for match in TOKEN.finditer(text):
        kind = match.lastgroup
        value = match.group()

        if kind == "string":
            out.append(("S", line))
        elif kind == "number":
            out.append(("N", line))
        elif kind in ("name", "punct", "other"):
            out.append((value, line))

        line += value.count("\n")

    return out


def window_hash(words: Sequence[str]) -> bytes:
    """A short, stable digest of a run of tokens."""
    return hashlib.blake2b("\x1f".join(words).encode(), digest_size=12).digest()


class Copy:
    """One repeated run: where it is in each file and how long it is."""

    def __init__(self, files: Tuple[str, str], lines: Tuple[Tuple[int, int], Tuple[int, int]],
                 length: int, key: str) -> None:
        self.files = files
        self.lines = lines
        self.length = length
        self.key = key

    def baseline_line(self) -> str:
        """The copy as its baseline line: both files and the hash of its tokens."""
        return f"{self.files[0]}\t{self.files[1]}\t{self.key}"

    def describe(self) -> str:
        """The copy for a person: both places and its length."""
        (a0, a1), (b0, b1) = self.lines
        return (f"{self.files[0]}:{a0}-{a1} and {self.files[1]}:{b0}-{b1} "
                f"({self.length} tokens)")


def find_copies(files: Dict[str, List[Token]], width: int) -> List[Copy]:
    """Every maximal run of at least `width` tokens that appears in two places."""
    names = sorted(files)
    words = {name: [t for t, _ in files[name]] for name in names}
    buckets: Dict[bytes, List[Tuple[int, int]]] = defaultdict(list)

    # Index every window of `width` tokens by its hash.
    for fid, name in enumerate(names):
        w = words[name]

        for i in range(len(w) - width + 1):
            buckets[window_hash(w[i:i + width])].append((fid, i))

    # Pair up the places that share a window, grouped by the offset between them so that the
    # windows of one copy fall into one group.
    pairs: Dict[Tuple[int, int, int], Set[int]] = defaultdict(set)

    for places in buckets.values():
        if len(places) < 2 or len(places) > BUCKET_LIMIT:
            continue

        for x in range(len(places)):
            for y in range(x + 1, len(places)):
                (fa, ia), (fb, ib) = places[x], places[y]

                if fa == fb and abs(ia - ib) < width:
                    continue

                pairs[(fa, fb, ib - ia)].add(ia)

    copies: List[Copy] = []

    # Consecutive window starts with one offset are one run.
    for (fa, fb, delta), starts in pairs.items():
        ordered = sorted(starts)
        run_start = prev = ordered[0]

        for ia in ordered[1:] + [None]:
            if ia is not None and ia == prev + 1:
                prev = ia
                continue

            copies.append(make_copy(names, files, words, fa, fb, run_start, prev + width, delta))

            if ia is not None:
                run_start = prev = ia

    return sorted(copies, key=lambda c: (-c.length, c.files, c.lines))


def make_copy(names: List[str], files: Dict[str, List[Token]], words: Dict[str, List[str]],
              fa: int, fb: int, start: int, end: int, delta: int) -> Copy:
    """The run [start, end) of file fa, which repeats at start + delta in file fb."""
    a, b = names[fa], names[fb]
    ta, tb = files[a], files[b]
    lines = ((ta[start][1], ta[end - 1][1]), (tb[start + delta][1], tb[end - 1 + delta][1]))
    key = window_hash(words[a][start:end]).hex()
    return Copy((a, b), lines, end - start, key)


def read_baseline() -> Set[str]:
    """The accepted copies, without their reasons."""
    if not BASELINE.exists():
        return set()

    accepted = set()

    for line in BASELINE.read_text().splitlines():
        if line and not line.startswith("#"):
            accepted.add("\t".join(line.split("\t")[:3]))

    return accepted


def write_baseline(copies: Iterable[Copy]) -> None:
    """Every copy found becomes an accepted one."""
    header = ("# Copies tools/dup_check.py accepts (docs/CODING_CONVENTIONS.md, 22.6): file, file,\n"
              "# hash of the repeated tokens, and optionally a tab and why the copy stays.\n")
    lines = sorted({c.baseline_line() for c in copies})
    BASELINE.write_text(header + "".join(line + "\n" for line in lines))


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0].strip())
    parser.add_argument("paths", nargs="*")
    parser.add_argument("--tokens", type=int, default=DEFAULT_TOKENS)
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--report-only", action="store_true")
    parser.add_argument("--write-baseline", action="store_true")
    args = parser.parse_args(argv)

    files: Dict[str, List[Token]] = {}

    for rel in tracked_sources(args.paths):
        text = (ROOT / rel).read_text(errors="replace")

        if not is_generated(text):
            files[rel] = tokens_of(text)

    copies = find_copies(files, args.tokens)

    if args.write_baseline:
        write_baseline(copies)
        print(f"dup_check: {len(copies)} copies written to {BASELINE.relative_to(ROOT)}")
        return 0

    accepted = set() if args.all else read_baseline()
    new = [c for c in copies if c.baseline_line() not in accepted]

    for copy in new:
        print(copy.describe())

    label = "copies" if args.all else "new copies"
    print(f"dup_check: {len(new)} {label} in {len(files)} files", file=sys.stderr)
    return 0 if args.report_only or not new else 1


if __name__ == "__main__":
    sys.exit(main())
