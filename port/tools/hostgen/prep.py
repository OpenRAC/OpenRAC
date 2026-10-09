# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The decompilation's sources, prepared for Clang to read as the console's C.

hostgen never changes the decompilation. It copies the files it reads into
its own build directory and changes the copies in ways that keep every line
where it was:

- `long` becomes `long long`: the EE's GCC has a 64-bit long, where Clang's
  32-bit MIPS target has a 32-bit one. `long long` stays as it is.
- File-scope `__asm__(...)` statements are blanked: they only lay out the
  matching build's object files (padding, sections), and GCC 2.95 accepted
  strings across lines in them, which Clang does not.
"""

from __future__ import annotations

import re
from pathlib import Path

# A C token that can contain the word long without being the keyword.
_SKIP = re.compile(
    r'"(?:\\.|[^"\\])*"'       # string literal (one line)
    r"|'(?:\\.|[^'\\])*'"      # character literal
    r"|/\*.*?\*/"              # block comment
    r"|//[^\n]*",              # line comment
    re.S,
)
_LONG = re.compile(r"\blong\b(\s+long\b)?")


def widen_long(text: str) -> str:
    """Every `long` that is not part of `long long` becomes `long long`."""
    out = []
    pos = 0
    for m in _SKIP.finditer(text):
        out.append(_widen(text[pos:m.start()]))
        out.append(m.group(0))
        pos = m.end()
    out.append(_widen(text[pos:]))
    return "".join(out)


def _widen(code: str) -> str:
    return _LONG.sub(lambda m: m.group(0) if m.group(1) else "long long", code)


def blank_file_scope_asm(text: str) -> str:
    """Blanks `__asm__(...);` statements that start a line at file scope."""
    chars = list(text)
    depth = 0          # braces: file scope is depth 0
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == "/" and text.startswith("/*", i):
            end = text.find("*/", i + 2)
            i = n if end < 0 else end + 2
            continue
        if c == "/" and text.startswith("//", i):
            end = text.find("\n", i)
            i = n if end < 0 else end
            continue
        if c in "\"'":
            i = _skip_literal(text, i)
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        elif depth == 0 and (i == 0 or text[i - 1] == "\n") and text.startswith("__asm__", i):
            end = _statement_end(text, i)
            if end is not None:
                for j in range(i, end):
                    if chars[j] != "\n":
                        chars[j] = " "
                i = end
                continue
        i += 1
    return "".join(chars)


def _skip_literal(text: str, i: int) -> int:
    quote = text[i]
    i += 1
    while i < len(text) and text[i] != quote:
        i += 2 if text[i] == "\\" else 1
    return i + 1


def _statement_end(text: str, i: int) -> int | None:
    """The index after the `;` that ends the asm statement starting at i."""
    j = text.find("(", i)
    if j < 0:
        return None
    depth = 0
    while j < len(text):
        c = text[j]
        if c in "\"'":
            j = _skip_literal(text, j)
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                k = j + 1
                while k < len(text) and text[k] in " \t\r\n":
                    k += 1
                return k + 1 if k < len(text) and text[k] == ";" else j + 1
        j += 1
    return None


def prepare(text: str) -> str:
    return widen_long(blank_file_scope_asm(text))


def copy_tree(src: Path, dst: Path, patterns=("*.c", "*.h", "*.inc")) -> int:
    """Prepares every C file under src into dst; only rewrites what changed."""
    count = 0
    for pattern in patterns:
        for path in src.rglob(pattern):
            out = dst / path.relative_to(src)
            text = prepare(path.read_text(encoding="utf-8", errors="surrogateescape"))
            if not out.exists() or out.read_text(encoding="utf-8", errors="surrogateescape") != text:
                out.parent.mkdir(parents=True, exist_ok=True)
                out.write_text(text, encoding="utf-8", errors="surrogateescape")
            count += 1
    return count
