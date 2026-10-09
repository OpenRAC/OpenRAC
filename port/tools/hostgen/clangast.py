# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Clang reads one translation unit as the console's C and gives its AST.

The unit is read for a 32-bit little-endian MIPS target, so every type has
the console's size and layout, in GNU C89 as GCC 2.95 accepted it. What
Clang refuses that GCC 2.95 accepted is fixed in the prepared copy, driven
by Clang's own diagnostics (fix_from_diagnostics), and recorded.
"""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from dataclasses import dataclass, field
from pathlib import Path

# GCC 2.95 accepted these; Clang makes them errors unless told otherwise.
RELAXED = [
    "-Wno-error=implicit-function-declaration",
    "-Wno-error=implicit-int",
    "-Wno-error=int-conversion",
    "-Wno-error=incompatible-pointer-types",
    "-Wno-error=incompatible-function-pointer-types",
    "-Wno-error=return-type",
    "-Wno-error=return-mismatch",
    "-Wno-error=int-to-pointer-cast",
]


def clang_binary() -> str:
    if os.environ.get("OPENRAC_CLANG"):
        return os.environ["OPENRAC_CLANG"]
    for name in ("clang", "clang-18", "clang-17", "clang-19", "clang-20"):
        path = shutil.which(name)
        if path:
            return path
    raise SystemExit("hostgen needs Clang (clang on PATH) to read the decompilation's C")


@dataclass
class Unit:
    path: Path                      # the prepared copy, relative paths inside
    ast: dict
    fixes: list[str] = field(default_factory=list)


def base_flags(includes: list[Path], defines: list[str]) -> list[str]:
    flags = [
        "--target=mipsel-linux-gnu", "-std=gnu89", "-fsyntax-only", "-w", "-ferror-limit=50",
        "-fsigned-char", "-nostdinc",
        # include_asm.h: no file-scope asm when M2CTX is set; INCLUDE_ASM marks the
        # function as one that is still assembly, which hostgen reads back.
        "-DM2CTX", "-DOPENRAC_HOSTGEN",
        "-DINCLUDE_ASM(FOLDER,NAME)=extern int openrac_asm_##NAME",
        "-DINCLUDE_RODATA(FOLDER,NAME)=",
    ] + RELAXED
    for inc in includes:
        flags += ["-I", str(inc)]
    for d in defines:
        flags.append(f"-D{d}")
    return flags


def run(clang: str, path: Path, cwd: Path, flags: list[str], json_out: bool) -> subprocess.CompletedProcess:
    cmd = [clang, *flags]
    if json_out:
        cmd += ["-Xclang", "-ast-dump=json"]
    cmd.append(str(path))
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, errors="replace")


_DIAG = re.compile(r"^(?P<file>[^:\n]+):(?P<line>\d+):(?P<col>\d+): (?P<kind>error|note): (?P<msg>.*)$", re.M)


def fix_from_diagnostics(text: str, stderr: str, rel: str) -> tuple[str, list[str]]:
    """Fixes, in the prepared copy, what Clang refuses and GCC 2.95 took.

    - "conflicting types for 'f'" after an implicit declaration: f is called
      before it is declared or defined in the same file, so GCC 2.95 gave
      that call an implicit `int f()`. The call is renamed to an alias with
      exactly that declaration and f's symbol (an asm label), declared at
      the start of the function that makes the call; f's own declaration
      and definition stay as they are.
    - "too few arguments to function call": a call to a K&R function with
      fewer arguments than its definition. The missing ones are passed as 0
      (on the console they were whatever the registers held).

    Both edits stay on their line, so line numbers do not move.
    """
    lines = text.split("\n")
    code_lines = _lines_outside_comments(text)
    fixes = []
    diags = list(_DIAG.finditer(stderr))
    inserts: dict[int, list[str]] = {}
    for k, d in enumerate(diags):
        if d["kind"] != "error" or not d["file"].endswith(rel):
            continue
        msg = d["msg"]
        line = int(d["line"]) - 1
        col = int(d["col"]) - 1
        m = re.match(r"conflicting types for '(\w+)'", msg)
        if m:
            name = m.group(1)
            note = diags[k + 1] if k + 1 < len(diags) else None
            if note is None or note["kind"] != "note" or "implicit declaration" not in note["msg"]:
                continue
            use, use_col = int(note["line"]) - 1, int(note["col"]) - 1
            src = lines[use]
            # The call may spell f through a macro (include/names.h).
            word = re.match(r"\w+", src[use_col:])
            if word is None:
                continue
            alias = f"{name}__openrac_implicit_{use + 1}"
            lines[use] = src[:use_col] + alias + src[use_col + word.end():]
            at = _function_start(lines, code_lines, use)
            inserts.setdefault(at, []).append(f'int {alias}() __asm__("{name}");')
            fixes.append(f"line {use + 1}: {name} called before its declaration, as int {name}()")
            continue
        m = re.match(r"too few arguments to function call, expected (\d+), have (\d+)", msg)
        if m:
            want, have = int(m.group(1)), int(m.group(2))
            src = lines[line]
            pad = ", ".join(["0"] * (want - have))
            if have:
                pad = ", " + pad
            if 0 <= col < len(src) and src[col] == ")":
                lines[line] = src[:col] + pad + src[col:]
                fixes.append(f"line {line + 1}: passed {want - have} missing argument(s) as 0")
    for at, decls in inserts.items():
        lines[at] = " ".join(dict.fromkeys(decls)) + " " + lines[at]
    return "\n".join(lines), fixes


def _lines_outside_comments(text: str) -> set[int]:
    """The lines that do not start inside a block comment."""
    inside = False
    out = set()
    for i, line in enumerate(text.split("\n")):
        if not inside:
            out.add(i)
        j = 0
        while j < len(line):
            if inside:
                end = line.find("*/", j)
                if end < 0:
                    break
                inside = False
                j = end + 2
            else:
                start = line.find("/*", j)
                slash = line.find("//", j)
                if start < 0 or (0 <= slash < start):
                    break
                inside = True
                j = start + 2
    return out


_FUNC_START = re.compile(r"^(?:static\s+|extern\s+)?[A-Za-z_][\w\s\*]*\b\w+\s*\(")


def _function_start(lines: list[str], code_lines: set[int], use: int) -> int:
    """The line that starts the function around line use (or line 0)."""
    for i in range(use, -1, -1):
        if i in code_lines and _FUNC_START.match(lines[i]) and not lines[i].rstrip().endswith(";"):
            return i
    return 0


def parse_unit(clang: str, root: Path, rel: str, flags: list[str], attempts: int = 12) -> tuple[Unit | None, str]:
    """The AST of root/rel. Returns (None, errors) if Clang still refuses it."""
    path = root / rel
    fixes: list[str] = []
    for _ in range(attempts):
        check = run(clang, Path(rel), root, flags, json_out=False)
        if check.returncode == 0:
            out = run(clang, Path(rel), root, flags, json_out=True)
            if out.returncode != 0:
                return None, out.stderr
            ast = json.loads(out.stdout)
            annotate(ast)
            return Unit(path, ast, fixes), ""
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        fixed, more = fix_from_diagnostics(text, check.stderr, rel)
        if not more or fixed == text:
            return None, check.stderr
        path.write_text(fixed, encoding="utf-8", errors="surrogateescape")
        fixes += more
    return None, "too many rounds of fixes"


def annotate(ast: dict) -> None:
    """Gives every node its file and line ("_file", "_line").

    Clang's JSON names a location's file and line only when they differ
    from the previous location it printed, so they are followed here in the
    order Clang wrote them.
    """
    state = {"file": "", "line": 0}

    def visit_loc(loc: dict | None):
        if not loc:
            return None
        if "spellingLoc" in loc:
            visit_loc(loc["spellingLoc"])
            return visit_loc(loc["expansionLoc"])
        if "file" in loc:
            state["file"] = loc["file"]
        if "line" in loc:
            state["line"] = loc["line"]
        return state["file"], state["line"], loc.get("col", 0)

    def walk(node: dict):
        where = visit_loc(node.get("loc"))
        rng = node.get("range")
        begin = None
        if rng:
            begin = visit_loc(rng.get("begin"))
            visit_loc(rng.get("end"))
        spot = where or begin
        if spot:
            node["_file"], node["_line"], node["_col"] = spot
        for child in node.get("inner", ()):
            walk(child)

    walk(ast)
