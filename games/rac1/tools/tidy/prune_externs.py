#!/usr/bin/env python3
"""
Delete file-scope extern declarations that nothing in the file uses.

  python3 tools/tidy/prune_externs.py src/game            # dry run: show counts
  python3 tools/tidy/prune_externs.py src/game --write
  python3 tools/tidy/prune_externs.py src/game --list     # show what would go

A name counts as used if it appears, as a word, anywhere else in the file's code, including:
  - strings (inline assembly names symbols there)
  - the file's version fragments (src/versions/<K>/<file>/), which are spliced into it
  - a macro body in include/, because a macro can use a declaration from the .c file
  - its address or readable name (include/names.h)

Kept on purpose: externs inside functions, ones with a provenance note beside them, and anything the parser
is unsure of. Run tools/check_all.sh after: only a build can prove the bytes did not change.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import names_header  # noqa: E402
WORD = re.compile(r"[A-Za-z_]\w*")
COMMENT_OR_STRING = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)
EXTERN = re.compile(r"\bextern\b")
DIRECTIVE = re.compile(r"^[ \t]*#(?:[^\n]*\\\n)*[^\n]*", re.M)
ATTRIBUTES = re.compile(r"\b(?:__asm__|__attribute__|SDATA)\s*(\((?:[^()]|\((?:[^()]|\([^()]*\))*\))*\))|\b(?:MACRO_ADDR|NOT_SDA)\b")
PROVENANCE = re.compile(r"Adapted|Lombyte|GPL|MIT\b|licen[cs]e|\(c\)", re.I)
KEYWORDS = {"extern", "const", "volatile", "unsigned", "signed", "struct", "union", "enum", "static", "void", "char",
            "short", "int", "long", "float", "double"}


def blank(match):
    """Spaces of the same length, keeping line breaks, so offsets stay right."""
    return re.sub(r"[^\n]", " ", match.group(0))


def mask(text):
    """Comments, strings and preprocessor lines blanked: what is left is plain code."""
    return DIRECTIVE.sub(blank, COMMENT_OR_STRING.sub(blank, text))


def without_comments(text):
    """Comments blanked, strings kept: inline assembly can name a symbol."""
    return COMMENT_OR_STRING.sub(lambda m: m.group(0) if m.group(0)[0] in "\"'" else blank(m), text)


def split_declarators(body):
    parts, depth, last = [], 0, 0
    for i, ch in enumerate(body):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif ch == "," and depth == 0:
            parts.append(body[last:i])
            last = i + 1
    parts.append(body[last:])
    return parts


def declared_names(statement):
    """Names a declaration declares, or None if the parser is unsure."""
    body = ATTRIBUTES.sub(" ", statement).rstrip(" ;\n\t")
    names = []
    for part in split_declarators(body):
        pointer = re.search(r"\(\s*\*+\s*(?:const\s+)?([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*\)", part)
        if pointer:
            names.append(pointer.group(1))
            continue
        head = re.split(r"[(\[]", part, maxsplit=1)[0]
        words = [w for w in WORD.findall(head) if w not in KEYWORDS]
        if not words:
            return None
        names.append(words[-1])
    return names


def find_externs(text):
    """(start, end, statement) for each extern statement at file scope."""
    masked = mask(text)
    depth_at = []
    depth = 0
    for ch in masked:
        depth_at.append(depth)
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
    found = []
    for m in EXTERN.finditer(masked):
        if depth_at[m.start()] != 0:
            continue
        end = masked.find(";", m.start())
        if end < 0 or "{" in masked[m.start():end]:
            continue                        # an extern with a body: leave it
        found.append((m.start(), end + 1, text[m.start():end + 1]))
    return found


def load_aliases():
    """{name: other names}, from include/names.h: readable <-> address."""
    aliases = {}
    for line in names_header.read(ROOT / "include/names.h").splitlines():
        m = re.match(r"#define\s+(\w+)\s+(\w+)\s*$", line)
        if m:
            aliases.setdefault(m.group(1), set()).add(m.group(2))
            aliases.setdefault(m.group(2), set()).add(m.group(1))
    return aliases


def header_macro_words():
    """Words in macro bodies in include/, except the generated name table."""
    words = set()
    for path in (ROOT / "include").rglob("*.h"):
        if path.name == "names.h" or path.parent.name == names_header.PARTS:
            continue
        for m in DIRECTIVE.finditer(COMMENT_OR_STRING.sub(blank, path.read_text(errors="replace"))):
            if re.match(r"\s*#\s*define\b", m.group(0)):
                words |= set(WORD.findall(m.group(0)))
    return words


def fragments_text(rel):
    """The version fragments spliced into this source file."""
    stem = rel[len("src/"):-len(".c")]
    return "\n".join(p.read_text(errors="replace") for p in sorted((ROOT / "src/versions").glob(f"*/{stem}/*.c")))


def trailing_comment_end(text, end):
    """End offset, past spaces and a comment on the same line as the declaration."""
    m = re.compile(r"[ \t]*(?:/\*(?:(?!\*/)[^\n])*\*/[ \t]*)?(?=\n|$)").match(text, end)
    return m.end() if m else end


def prune(text, fragments, aliases, macro_words):
    """(new text, removed statements, kept for provenance)."""
    statements = find_externs(text)
    spans = [(s, e) for s, e, _ in statements]
    # Everything that is not one of these statements counts as a use.
    kept_parts, last = [], 0
    for start, end in spans:
        kept_parts.append(text[last:start])
        last = end
    kept_parts.append(text[last:])
    used = set(WORD.findall(without_comments("".join(kept_parts) + "\n" + fragments))) | macro_words

    remove, removed, provenance = [], [], 0
    for start, end, statement in statements:
        names = declared_names(statement)
        if names is None:
            continue
        related = set(names)
        for name in names:
            related |= aliases.get(name, set())
        if related & used:
            continue
        stop = trailing_comment_end(text, end)
        if PROVENANCE.search(text[start:stop]):
            provenance += 1
            continue
        remove.append((start, stop))
        removed.append(statement)
    out, last = [], 0
    for start, stop in remove:
        out.append(text[last:start])
        last = stop
    out.append(text[last:])
    # A removed line leaves only its indent behind.
    result = re.sub(r"^[ \t]+$", "", "".join(out), flags=re.M) if remove else text
    return (re.sub(r"\n{3,}", "\n\n", result) if remove else text), removed, provenance


def sources(paths):
    out = subprocess.run(["git", "ls-files", "--", *paths], cwd=ROOT, capture_output=True, text=True, check=True).stdout
    return [f for f in out.split() if f.endswith(".c") and not f.startswith(("src/versions/", "src/libgcc/"))]


def main():
    args = sys.argv[1:]
    write, listing = "--write" in args, "--list" in args
    paths = [a for a in args if not a.startswith("--")]
    if not paths:
        sys.exit(__doc__)
    aliases, macro_words = load_aliases(), header_macro_words()
    total = kept_notes = 0
    for rel in sources(paths):
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        new, removed, provenance = prune(text, fragments_text(rel), aliases, macro_words)
        total += len(removed)
        kept_notes += provenance
        if listing:
            for statement in removed:
                print(f"{rel}: {' '.join(statement.split())[:110]}")
        if write and new != text:
            path.write_text(new, errors="surrogateescape")
    print(f"{'removed' if write else 'would remove'} {total} declarations; kept {kept_notes} with a provenance note")


if __name__ == "__main__":
    main()
