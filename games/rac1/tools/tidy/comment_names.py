#!/usr/bin/env python3
"""
Use readable names in comments, and remove workers' status tags.

  python3 tools/tidy/comment_names.py            # dry run: show counts
  python3 tools/tidy/comment_names.py --write
  python3 tools/tidy/comment_names.py --list     # show the changes

`func_001F4E08` in a comment becomes its name from include/names/. A symbol with no name stays. Everything from a
provenance note onward is left alone: those are licence credit, and names there are the source project's.

Comment-only lines over 120 characters are wrapped. A trailing comment after code stays, and so does a line with a
provenance note or a FAKEMATCH or KEPT marker.

`functional: medium.` at the start of a comment goes, and the description after it stays.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
ADDRESS = re.compile(r"\b(?:func|D)_(?:L\d+_)?[0-9A-Fa-f]{5,}(?:_r\d*)?\b")
PROVENANCE = re.compile(r"Adapted from|Lombyte|GPL|\(c\)|NTSC decomp", re.I)
STATUS = re.compile(r"\bfunctional: (?:high|medium|low)\.\s*")
SPELLING = {"Mcmarx": "McMarx"}
MARKER = re.compile(r"\b(?:FAKEMATCH|KEPT):")
WIDTH = 120


def readable_names():
    """{address name: readable name}"""
    names = {}
    for name, others in pe.load_aliases().items():
        if re.fullmatch(r"(?:func|D)_(?:L\d+_)?[0-9A-Fa-f]{5,}", name):
            readable = [other for other in others if not other.startswith(("func_", "D_"))]
            if len(readable) == 1:
                names[name] = readable[0]
    return names


def rewrite_comment(comment, names):
    start = PROVENANCE.search(comment)
    head, tail = (comment[:start.start()], comment[start.start():]) if start else (comment, "")

    def one(m):
        return names.get(m.group(0), m.group(0))

    head = ADDRESS.sub(one, head)
    # "cd_callback (cd_callback)": the comment already said it.
    head = re.sub(r"\b(\w+) \(\1\)", r"\1", head)
    head = STATUS.sub("", head)
    for wrong, right in SPELLING.items():
        head = head.replace(wrong, right)
    return head + tail


def rewrite(text, names):
    """(new text, number of comments changed)."""
    count = [0]

    def one(m):
        if m.group(0)[0] != "/":
            return m.group(0)
        new = rewrite_comment(m.group(0), names)
        count[0] += new != m.group(0)
        return new

    return pe.COMMENT_OR_STRING.sub(one, text), count[0]


def wrap_line(line, width=WIDTH):
    """A comment-only line over the width, as several lines, or the line itself if it should stay."""
    if len(line) <= width or PROVENANCE.search(line) or MARKER.search(line):
        return [line]
    m = re.match(r"(\s*)(/\*+\s|\*\s|//\s?)?(.*)$", line)
    indent, opener, body = m.group(1), m.group(2) or "", m.group(3)
    if opener in ("", "// ", "//") and not opener:
        pad = indent
    elif opener.startswith("/*"):
        pad = indent + " " * len(opener)
    elif opener.startswith("*"):
        pad = indent + "* "
    else:
        pad = indent + opener
    out, current = [], indent + opener
    for word in body.split(" "):
        if len(current) + len(word) + 1 > width and current.strip() not in ("", "*", "/*", "//"):
            out.append(current.rstrip())
            current = pad
        current += word + " "
    out.append(current.rstrip())
    return out if all(len(part) <= width for part in out) else [line]


def wrap_comments(text):
    """Wrap comment-only lines. A trailing comment after code stays, and so does a line with a long word."""
    masked = pe.COMMENT_OR_STRING.sub(lambda m: pe.blank(m) if m.group(0)[0] == "/" else m.group(0), text)
    out = []
    for line, code in zip(text.split("\n"), masked.split("\n")):
        out.extend(wrap_line(line) if not code.strip() and line.strip() else [line])
    return "\n".join(out)


def main():
    write, listing = "--write" in sys.argv[1:], "--list" in sys.argv[1:]
    names = readable_names()
    out = __import__("subprocess").run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout
    total = files = 0
    for rel in out.split():
        if rel.startswith(("src/libgcc/", "src/data/")):
            continue
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        new, count = rewrite(text, names)
        wrapped = wrap_comments(new)
        count += wrapped != new
        new = wrapped
        if not count:
            continue
        total += count
        files += 1
        if listing:
            old_lines, new_lines = text.split("\n"), new.split("\n")
            for old, line in zip(old_lines, new_lines):
                if old != line:
                    print(f"{rel}: {line.strip()[:110]}")
        if write:
            path.write_text(new, errors="surrogateescape")
    print(f"{'changed' if write else 'would change'} {total} comments in {files} files")


if __name__ == "__main__":
    main()
