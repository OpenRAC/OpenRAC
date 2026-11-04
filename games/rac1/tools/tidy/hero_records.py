#!/usr/bin/env python3
"""
Replace local copies of two hero records with their shared types.

  python3 tools/tidy/hero_records.py            # dry run: show counts
  python3 tools/tidy/hero_records.py --write

`HS` is struct CollisionHit (structs/collision.h), and `T_8` is struct HitMessage (structs/hit_message.h). Only a copy
with the known body converts, and only fields reached through a variable of that type are renamed. Run tools/format.py
and tools/gate_loop.py afterwards.
"""
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
HS_BODY = "u8 pad0[0x18]; s32 i18; s32 i1C; union { Q q; f32 f[4]; } v20; u8 pad30[0x10]; f32 f40; f32 f44;"
T8_BODY = "u8 pad0[0x10]; Q_8 v10; u8 *p20; s32 i24; u8 b28; u8 pad29[3]; f32 f2C; s32 i30; u8 *p34; u8 pad38[8];"
RECORDS = {
    "HS": ("CollisionHit", "structs/collision.h", HS_BODY,
           {"i18": "moby", "i1C": "unk1C", "v20": "point", "f40": "normal.f[0]", "f44": "normal.f[1]"}),
    "T_8": ("HitMessage", "structs/hit_message.h", T8_BODY,
            {"v10": "unk_10", "p20": "source", "i24": "flags", "b28": "unk_28", "f2C": "damage", "i30": "unk_30",
             "p34": "target"}),
}
BODY = r"typedef\s+struct(?:\s+\w+)?\s*\{((?:[^{}]|\{[^{}]*\})*)\}\s*NAME\s*;[ \t]*\n?"


def normal(body):
    return " ".join(re.sub(r"/\*.*?\*/", "", body, flags=re.S).split())


def variables(text, name):
    return sorted(set(re.findall(rf"\b{name}\s+\*?\s*(\w+)\s*(?:[\[;=,)]|__asm__)", pe.without_comments(text))))


def convert(text):
    """(new text, names of the records converted)."""
    done = []
    for name, (new, header, body, fields) in RECORDS.items():
        m = re.search(BODY.replace("NAME", name), text)
        if not m or normal(m.group(1)) != body:
            continue
        names = variables(text[:m.start()] + text[m.end():], name)
        text = text[:m.start()] + text[m.end():]
        for variable in names:
            pattern = rf"\b{variable}(\s*(?:\.|->)\s*)({'|'.join(fields)})\b"
            text = re.sub(pattern, lambda f: f"{variable}{f.group(1)}{fields[f.group(2)]}", text)
        text = re.sub(rf"\b{name}\b", f"struct {new}", text)
        if f'"{header}"' not in text:
            use = text.index(f"struct {new}")
            heads = list(re.finditer(r'^#include\s+"[^"]+"', text[:use], re.M))
            at = text.index("\n", heads[-1].end()) + 1 if heads else 0
            text = text[:at] + f'#include "{header}"\n' + text[at:]
        done.append(name)
    return text, done


def main():
    write = "--write" in sys.argv[1:]
    paths = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout.split()
    counts = {name: 0 for name in RECORDS}
    for rel in paths:
        path = ROOT / rel
        new, done = convert(path.read_text(errors="surrogateescape"))
        for name in done:
            counts[name] += 1
        if done and write:
            path.write_text(new, errors="surrogateescape")
    print(f"{'converted' if write else 'would convert'} {counts}")


if __name__ == "__main__":
    main()
