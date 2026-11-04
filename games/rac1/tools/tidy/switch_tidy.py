#!/usr/bin/env python3
"""
Make switch statements easier to read, without touching what they compile to.

  python3 tools/tidy/switch_tidy.py            # dry run: show counts
  python3 tools/tidy/switch_tidy.py --write
  python3 tools/tidy/switch_tidy.py --list     # show each fall-through

A case that runs into the next one gets `/* fall through */`, so it does not look like a bug. Case numbers in one switch
use one base. The order of the cases stays: it is what gives retail's jump table. Empty cases stay too: see docs/STYLE.md.
"""
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
CASE = re.compile(r"\b(?:case\s+[^:\n]+|default)\s*:")
NUMBER = re.compile(r"(case\s+)(0[xX][0-9A-Fa-f]+|\d+)(\s*:)")
MARK = "/* fall through */"


def mask(text):
    return pe.COMMENT_OR_STRING.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)


def closing(code, start):
    depth = 0
    for at in range(start, len(code)):
        if code[at] == "{":
            depth += 1
        elif code[at] == "}":
            depth -= 1
            if not depth:
                return at
    return len(code) - 1


def leaves(code):
    """True if control never reaches the end of this code. Unsure means False only when it plainly can run on."""
    code = code.strip()
    if not code:
        return False
    if code.endswith(";"):
        tail = code[code.rfind(";", 0, len(code) - 1) + 1:].strip() if ";" in code[:-1] else code
        tail = tail[tail.rfind("}") + 1:].strip()
        return bool(re.match(r"(?:break|continue|goto\b|return\b)", tail))
    if not code.endswith("}"):
        return False
    depth, at = 0, len(code) - 1
    while at >= 0:
        if code[at] == "}":
            depth += 1
        elif code[at] == "{":
            depth -= 1
            if not depth:
                break
        at -= 1
    inner, head = code[at + 1:-1], code[:at].rstrip()
    statement = re.search(r"(?:^|[;{}])\s*((?:else\s+)?(?:if|while|for|do|switch)\b[^;{}]*)$", head) or None
    if statement is None:
        return leaves(inner) if not head.endswith(")") and not head.endswith("else") else False
    keyword = statement.group(1)
    if keyword.startswith(("while", "for", "do", "switch")):
        # A loop that never ends leaves, and any other can run on. One we cannot judge gets no mark.
        return None if re.match(r"(?:while\s*\(\s*1\s*\)|for\s*\(\s*;\s*;\s*\))", keyword) else (None if "switch" in keyword or keyword.startswith("do") else False)
    if keyword.startswith("if"):
        return False
    # else { ... }: every arm of the chain has to leave.
    return None


def own_labels(code, start, end):
    """(label start, label end) of this switch's own labels, leaving nested switches out."""
    labels, depth, at = [], 0, start
    while at < end:
        c = code[at]
        nested = re.compile(r"\bswitch\s*\(").match(code, at)
        if nested:
            brace = code.find("{", nested.end())
            at = closing(code, brace) + 1
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        elif depth == 0:
            label = CASE.match(code, at)
            if label:
                labels.append((at, label.end()))
                at = label.end()
                continue
        at += 1
    return labels


def rewrite(text):
    """(new text, fall-throughs marked, switches with one base now, [(line, label)])."""
    code = mask(text)
    inserts, base_edits, found = [], [], []
    fixed = 0
    for m in re.finditer(r"\bswitch\s*\(", code):
        brace = code.find("{", m.end())
        if brace < 0:
            continue
        end = closing(code, brace)
        labels = own_labels(code, brace + 1, end)
        for (start, stop), (following, _) in zip(labels, labels[1:]):
            body = code[stop:following]
            if not body.strip() or leaves(body) is not False:
                continue
            if "fall" in text[stop:following].lower():
                continue
            line = text.rfind("\n", 0, following) + 1
            indent = text[line:following]
            if indent.strip():
                continue
            inserts.append((line, f"{indent}    {MARK}\n"))
            found.append((text.count("\n", 0, following) + 1, text[start:stop]))
        numbers = [(s, NUMBER.match(code, s)) for s, _ in labels]
        numbers = [(s, n) for s, n in numbers if n]
        hexes = sum(n.group(2).lower().startswith("0x") for _, n in numbers)
        small = len(numbers) - hexes
        if hexes and small:
            as_hex = hexes >= small
            for _, n in numbers:
                value = int(n.group(2), 0)
                new = f"0x{value:X}" if as_hex else str(value)
                if new != n.group(2):
                    base_edits.append((n.start(2), n.end(2), new))
            fixed += 1
    edits = [(at, at, new) for at, new in inserts] + base_edits
    out, last = [], 0
    for start, end, new in sorted(set(edits)):
        out += [text[last:start], new]
        last = end
    out.append(text[last:])
    return "".join(out), len(set(inserts)), fixed, found


def main():
    write, listing = "--write" in sys.argv[1:], "--list" in sys.argv[1:]
    out = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout
    marks = fixed = 0
    for rel in out.split():
        if rel.startswith(("src/data/", "src/libgcc/")):
            continue
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        new, m, f, found = rewrite(text)
        marks += m
        fixed += f
        if listing:
            for line, label in found:
                print(f"{rel}:{line}: {label}")
        if write and new != text:
            path.write_text(new, errors="surrogateescape")
    print(f"{'marked' if write else 'would mark'} {marks} fall-throughs; {fixed} switches get one number base")


if __name__ == "__main__":
    main()
