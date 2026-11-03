#!/usr/bin/env python3
"""
Replace local views of a voice slot with one shared type.

  python3 tools/tidy/voice_rows.py            # dry run: show counts
  python3 tools/tidy/voice_rows.py --write [FILE...]
  python3 tools/tidy/voice_rows.py --list

`Row13E650 *e = (Row13E650 *)(voice_pool + i * 0x70);` and its `e->f74` become VoicePoolRow (include/structs/voice.h)
and `e->state`. A local struct converts when the code casts voice_pool to it and every field it names sits at an offset
the shared type has. Only fields reached through a pointer of that type are renamed. --write checks each file with
fake_bases.check and restores any file that stops matching. Run tools/format.py and tools/gate_loop.py afterwards.
"""
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fake_bases as fb  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
HEADER = "structs/voice.h"
FIELDS = {0x70: "handle", 0x74: "state", 0x75: "flags", 0x78: "definition", 0x7C: "source_value", 0x7E: "linked_index",
          0x80: "volume", 0x84: "pitch_bend", 0x88: "owner", 0x8C: "reserved8C", 0x90: "position"}
SIZES = {"char": 1, "u8": 1, "unsigned char": 1, "short": 2, "s16": 2, "u16": 2, "int": 4, "s32": 4, "u32": 4,
         "float": 4, "Vec4f": 16}
FIELD = re.compile(r"^((?:unsigned\s+|signed\s+|struct\s+)?\w+)\s*(\*?)\s*(\w+)(?:\[([^\]]+)\])?$")
DEFINITION = r"(?:typedef\s+)?struct(?:\s+\w+)?\s*\{([^{}]*)\}\s*NAME\s*;[ \t]*\n?|struct\s+NAME\s*\{([^{}]*)\}\s*;[ \t]*\n?"
CAST = r"\(\s*(?:struct\s+)?(\w+)\s*\*\s*\)\s*\(?\s*(?:\(\s*\w+(?:\s+\w+)?\s*\*\s*\))?voice_pool(?:_r)?\s*\+"


def layout(body):
    """{old field name: offset}, or None if the body has a field this cannot size."""
    offset, found = 0, {}
    for field in re.sub(r"/\*.*?\*/", "", body, flags=re.S).split(";"):
        field = " ".join(field.split())
        if not field:
            continue
        m = FIELD.match(field)
        if not m:
            return None
        kind, star, name, count = m.groups()
        size = 4 if star else SIZES.get(kind)
        if size is None:
            return None
        try:
            total = size * (eval(count, {"__builtins__": {}}) if count else 1)
        except (SyntaxError, NameError):
            return None
        if size == 4:
            offset = (offset + 3) & ~3
        if not (count and size == 1):       # an array of bytes is padding
            found[name] = offset
        offset += total
    return found


def field_map(body):
    """{old field: new field} for the named fields, or None if a field has no place in the shared type."""
    fields = layout(body)
    if fields is None:
        return None
    named = {}
    for name, offset in fields.items():
        if offset not in FIELDS:
            return None
        named[name] = FIELDS[offset]
    return named


def definitions(text, name):
    return list(re.finditer(DEFINITION.replace("NAME", re.escape(name)), text))


def block_end(text, start):
    """Where the block holding `start` closes."""
    masked = pe.COMMENT_OR_STRING.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)
    depth = 0
    for i in range(start, len(masked)):
        if masked[i] == "{":
            depth += 1
        elif masked[i] == "}":
            if depth == 0:
                return i
            depth -= 1
    return len(text)


def rename_fields(text, name, fields):
    """Rename fields reached through a pointer of this type, then the type."""
    pieces = sorted(fields, key=len, reverse=True)
    if pieces:
        alternatives = "|".join(map(re.escape, pieces))
        declaration = re.compile(rf"(?:struct\s+)?\b{re.escape(name)}\s*\*\s*(\w+)\s*[=;,)]")
        for m in reversed(list(declaration.finditer(text))):
            variable = m.group(1)
            end = block_end(text, m.end())
            scope = re.sub(rf"\b{re.escape(variable)}->({alternatives})\b", lambda f: f"{variable}->{fields[f.group(1)]}",
                           text[m.end():end])
            text = text[:m.end()] + scope + text[end:]
        text = re.sub(rf"(\(\s*(?:\w+\s*=\s*)?\(\s*(?:struct\s+)?{re.escape(name)}\s*\*\s*\)[^;]*?\)->)({alternatives})\b",
                      lambda f: f.group(1) + fields[f.group(2)], text)
    return re.sub(rf"(?:struct\s+)?\b{re.escape(name)}\b", "VoicePoolRow", text)


def tidy_compares(text):
    """A cast only made the old field's type fit."""
    return re.sub(r"(->owner\s*==\s*)\(\(unsigned char \*\)(g_hero\.\w+)\)", r"\1\2", text)


def add_include(text):
    """After the last include above the first use: a file can have includes further down."""
    if f'"{HEADER}"' in text:
        return text
    last = None
    use = text.index("VoicePoolRow")
    for m in re.finditer(r'^#include\s+"[^"]+"', text[:use], re.M):
        last = m
    line = f'#include "{HEADER}"\n'
    if not last:
        return line + text
    at = text.index("\n", last.end()) + 1
    return text[:at] + line + text[at:]


def convert(text, fragments=""):
    """(new text, {type: field map}) for this file."""
    done = {}
    for name in sorted(set(re.findall(CAST, pe.without_comments(text)))):
        found = definitions(text, name)
        if len(found) != 1:
            continue
        fields = field_map(found[0].group(1) or found[0].group(2))
        if fields is None:
            continue
        done[name] = fields
    for name, fields in done.items():
        found = definitions(text, name)[0]
        text = text[:found.start()] + text[found.end():]
        text = rename_fields(text, name, fields)
    return (tidy_compares(add_include(text)) if done else text), done


def land(rel):
    path = ROOT / rel
    original = path.read_text(errors="surrogateescape")
    fragments = fb.fragments_of(rel)
    new, done = convert(original)
    if not done:
        return 0
    path.write_text(new, errors="surrogateescape")
    bad, _, failed = fb.check(rel)
    if bad or failed:
        path.write_text(original, errors="surrogateescape")
        print(f"{rel}: stays ({len(bad)} functions)", flush=True)
        return 0
    for fragment in fragments:
        old = fragment.read_text(errors="surrogateescape")
        changed = old
        for name, fields in done.items():
            changed = rename_fields(changed, name, fields)
        if changed != old:
            fragment.write_text(tidy_compares(changed), errors="surrogateescape")
    return len(done)


def main():
    args = sys.argv[1:]
    write, listing = "--write" in args, "--list" in args
    paths = [a for a in args if not a.startswith("--")]
    if not paths:
        paths = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout.split()
    total = files = 0
    for rel in paths:
        if rel.startswith(("src/versions/", "src/data/")):
            continue
        if write:
            count = land(rel)
        else:
            _, done = convert((ROOT / rel).read_text(errors="surrogateescape"))
            count = len(done)
            if listing and done:
                print(f"{rel}: {', '.join(done)}")
        total += count
        files += bool(count)
    print(f"{'converted' if write else 'would convert'} {total} types in {files} files")


if __name__ == "__main__":
    main()
