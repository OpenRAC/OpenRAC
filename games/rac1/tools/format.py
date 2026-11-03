#!/usr/bin/env python3
"""
Format the C in src/ and include/ with clang-format.

  python3 tools/format.py                  # format everything
  python3 tools/format.py src/game         # format one folder or file
  python3 tools/format.py --check          # fail if a file is not formatted, or the tools could not read it

Formatting must change only whitespace, so each file's text without whitespace is compared before and
after. A file that fails is put back.

The tools read source with line regexes, so --check also fails on:
  - an INCLUDE_ASM, ASM_FUNC or LINKER_REMNANT that does not start its line
  - a function whose return type is alone on the line above its name
  - a prototype without extern wrapped over two lines: its first line looks like a definition
  - an old-style (K&R) definition joined on one line
"""
import concurrent.futures
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VERSION = "22.1"
# include/names.h and include/names/ are generated, src/libgcc is verbatim GCC source, and these stay as upstream wrote them.
SKIP = ("include/names.h", "include/names/", "src/libgcc/")
STRING = re.compile(r'"(?:\\.|[^"\\\n])*"')
MARKER = re.compile(r"^([ \t]+)(INCLUDE_ASM|ASM_FUNC|LINKER_REMNANT)\s*\(", re.M)
# A prototype without extern, wrapped: its first line looks like a function definition.
WRAPPED_PROTO = re.compile(r"^(?!extern\b|typedef\b|#|__asm__|struct\b[^(]*\{)[A-Za-z_][^;{}()\n=]*\([^;{}]*?\n[^;{}]*?\)[^;{}\n]*;", re.M)
# An old-style definition joined on one line. Keep these in /* clang-format off */.
JOINED_KR = re.compile(r"^(?!extern\b|typedef\b)[A-Za-z_][^;{}=\n]*\([A-Za-z_0-9, ]+\) +[A-Za-z_][^;(){}\n]*;[ \t]*\n\{", re.M)
SPLIT_NAME = re.compile(r"^(?:static\s+|const\s+|unsigned\s+|struct\s+)*[A-Za-z_]\w*[ \t*]*\n[A-Za-z_]\w*\s*\([^;]*$", re.M)


def clang_format():
    """clang-format of the pinned version: other versions format differently."""
    for name in (f"clang-format-{VERSION.split('.')[0]}", "clang-format"):
        try:
            out = subprocess.run([name, "--version"], capture_output=True, text=True).stdout
        except FileNotFoundError:
            continue
        if f"version {VERSION}" in out:
            return name
    sys.exit(f"*** clang-format {VERSION} not found (pip install clang-format=={VERSION}.*)")


def sources(paths=()):
    args = ["git", "ls-files", "-co", "--exclude-standard", "--", *(paths or ["src", "include"])]
    out = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, check=True).stdout
    return [f for f in out.split("\n") if f.endswith((".c", ".h")) and not f.startswith(SKIP) and (ROOT / f).exists()]


def squeeze(text):
    """The text with whitespace removed, except inside strings, where it counts."""
    text = text.replace("\\\n", "\n")        # a macro's line continuations may move
    parts, last = [], 0
    for m in STRING.finditer(text):
        parts.append(re.sub(r"\s+", "", text[last:m.start()]))
        parts.append(m.group(0))
        last = m.end()
    parts.append(re.sub(r"\s+", "", text[last:]))
    return "".join(parts)


def run_format(tool, rel, text):
    result = subprocess.run([tool, "--style=file", f"--assume-filename={ROOT / rel}"], input=text, cwd=ROOT,
                            capture_output=True, text=True, errors="surrogateescape")
    if result.returncode:
        sys.exit(f"*** {rel}: {result.stderr[:300]}")
    return result.stdout


def tool_problems(text):
    """Why the line-based tools could not read this text."""
    problems = [f"{m.group(2)} does not start its line" for m in MARKER.finditer(text)]
    problems += ["return type on its own line: " + m.group(0).split("\n")[0] for m in SPLIT_NAME.finditer(text)]
    problems += ["wrapped prototype needs extern: " + m.group(0).split("\n")[0] for m in WRAPPED_PROTO.finditer(text)]
    problems += ["old-style definition joined: " + m.group(0).split("\n")[0] for m in JOINED_KR.finditer(text)]
    return problems


def format_one(job):
    tool, rel, check = job
    path = ROOT / rel
    text = path.read_text(errors="surrogateescape")
    formatted = run_format(tool, rel, text)
    if squeeze(formatted) != squeeze(text):
        return rel, ["formatting changed more than whitespace"]
    if check:
        return rel, (["not formatted"] if formatted != text else []) + tool_problems(text)
    if formatted != text:
        path.write_text(formatted, errors="surrogateescape")
    return rel, tool_problems(formatted)


def main():
    args = sys.argv[1:]
    check = "--check" in args
    paths = [a for a in args if not a.startswith("--")]
    tool = clang_format()
    files = sources(paths)
    with concurrent.futures.ThreadPoolExecutor() as pool:
        found = [(rel, why) for rel, why in pool.map(format_one, [(tool, f, check) for f in files]) if why]
    for rel, why in found[:40]:
        print(f"{rel}: {'; '.join(why)[:200]}")
    if found:
        sys.exit(f"*** {len(found)} of {len(files)} files have problems")
    print(f"{len(files)} files {'are formatted' if check else 'formatted'}")


if __name__ == "__main__":
    main()
