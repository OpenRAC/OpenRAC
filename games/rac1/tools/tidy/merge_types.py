#!/usr/bin/env python3
"""
Replace copies of a local type with one shared definition under a real name.

  python3 tools/tidy/merge_types.py            # dry run: show counts
  python3 tools/tidy/merge_types.py --write

MERGES below lists each shared type: its new name, its header, its definition, and which local types it
replaces. A local type is replaced only if its name matches and its body is listed exactly. Any other body
under that name stays as it is.

Each merged body must have the same size and field offsets as the definition. Names and field spellings have
no bytes, so run tools/check_all.sh to prove that.
"""
import collections
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prune_externs as pe  # noqa: E402
import prune_types as pt  # noqa: E402

ROOT = pe.ROOT
HERO = "structs/hero_views.h"
LEVEL_S17 = r"S17[0-9A-F]{4}"


def row(new, header, definition, names, bodies):
    return {"new": new, "header": header, "definition": definition, "names": names, "bodies": bodies}


MERGES = [
    row("IntPair", "structs/small_records.h", "int a; int b;", "Pair8|Pair", ["int a, b;"]),
    row("QuadUv", "structs/small_records.h", "float u; float v;", "UV_2bb0b0", ["float u, v;"]),
    row("UvScrollFactors", "structs/small_records.h", "float f[4];", "Quad4", ["float f[4];"]),
    row("IntVec6", "structs/small_records.h", "int v[6];", "V6", ["int v[6];"]),
    row("HeroIdleFidget", HERO, "char p0[0x44]; int f44; int f48; char p1[0x10]; float f5C; int f60; char p2[0xC];",
        "Rec70", ["char p0[0x44]; int f44; int f48; char p1[0x10]; float f5C; int f60; char p2[0xC];"]),
    row("HeroPlayerCameraView", HERO,
        "char p0[0x158]; float f158; char p15C[4]; float f160; char p164[4]; int f168; char p16C[0x1E4]; float f350[4];",
        "Cam", ["char p0[0x158]; float f158; char p15C[4]; float f160; char p164[4]; int f168; char p16C[0x1E4]; "
                "float f350[4];"]),
    row("HeroBoneRow", HERO, "char p0[0x60]; float f60[4]; char p70[0x30]; float fA0[4];",
        "Bone", ["char p0[0x60]; float f60[4]; char p70[0x30]; float fA0[4];"]),
    row("HeroBoneScalars", HERO, "char p0[0x254]; float f254; float f258;",
        LEVEL_S17, ["char p0[0x254]; float f254; float f258;"]),
    # 0x1C bytes. The second body names the same fields with f8 left in the padding.
    row("HeroSpeedBands", HERO, "char p0[8]; float f8; float fC; char p10[8]; float f18;",
        LEVEL_S17, ["char p0[8]; float f8; float fC; char p10[8]; float f18;",
                                  "char p0[0xC]; float fC; char p1[8]; float f18;"]),
    # 0x2C bytes: every field any level reads.
    row("HeroAttackRecord", HERO, "char p0[4]; int f4; char p8[4]; int fC; int f10; int f14; int f18; int f1C; int f20; "
        "char p24[8];",
        "Rec2C", ["char p0[4]; int f4; char p8[0x10]; int f18; int f1C; int f20; char p24[8];",
                  "char p0[0xC]; int fC; char p1[4]; int f14; int f18; char p2[0x10];",
                  "char p0[0xC]; int fC; int f10; int f14; int f18; char p2[0x10];"]),
    # 0x4C bytes: every field any level reads.
    row("HeroItemDefView", HERO, "char p0[0x18]; int f18; char p1C[4]; int f20; int f24; char p28[0x24];",
        "Rec4C|Rec20", ["char p0[0x24]; int f24; char p1[0x24];", "char p0[0x18]; int f18; char p1C[0x30];",
                        "char pad[0x20]; int f20; char pad2[0x28];"]),
]


# Types with good names already: moved as written, into a header per topic. Listed in dependency order.
VERBATIM = {
    "core/newlib_types.h": ["__sbuf_nl", "_atexit_nl", "_reent_nl", "FILE_nl", "Bigint_1154D0", "MallocChunk"],
    "structs/gs.h": ["GifTag_FAB40", "GsFbp_FAB40", "FsAa_FAB40"],
    "structs/swarm.h": ["SwarmPath", "SwarmPathFollower"],
    "structs/anim.h": ["AnimClass", "MobyAnim"],
    "structs/level_records.h": ["FogPreset", "MusicCdReadMode", "QuadPkt"],
}
VERBATIM_INCLUDES = '#include "common.h"\n#include "structs/vector.h"\n#include "structs/records.h"\n'


def statement_body(text, start, end):
    masked = pe.mask(text)[start:end]
    return re.sub(r"\s+", " ", masked[masked.index("{") + 1:masked.rindex("}")]).strip()


def is_plain_typedef(text, start, end):
    """typedef struct { ... } Name; with no tag and no attribute."""
    masked = pe.mask(text)[start:end]
    return bool(re.fullmatch(r"typedef\s+struct\s*\{.*\}\s*\w+\s*;", masked, re.S))


def find_replacements(text):
    """[(start, stop, old name, merge row)] of local types a merge replaces."""
    found = []
    for start, end, names in pt.find_types(text):
        if len(names) != 1 or not is_plain_typedef(text, start, end):
            continue
        name = next(iter(names))
        body = statement_body(text, start, end)
        for merge in MERGES:
            if re.fullmatch(merge["names"], name) and body in merge["bodies"]:
                found.append((start, pe.trailing_comment_end(text, end), name, merge))
                break
    return found


def rename_code(text, renames):
    """Rename whole words in code, leaving comments, strings and struct fields alone."""
    if not renames:
        return text
    pattern = re.compile(r"(?<![.\w])(?<!->)\b(" + "|".join(map(re.escape, sorted(renames, key=len, reverse=True))) + r")\b")
    out, last = [], 0
    for m in pe.COMMENT_OR_STRING.finditer(text):
        out.append(pattern.sub(lambda w: renames[w.group(1)], text[last:m.start()]))
        out.append(m.group(0))
        last = m.end()
    out.append(pattern.sub(lambda w: renames[w.group(1)], text[last:]))
    return "".join(out)


def verbatim_blocks(text):
    """{name: (start, stop, whole statement)} for the types in VERBATIM defined in this file."""
    wanted = {name for names in VERBATIM.values() for name in names}
    found = {}
    for start, end, names in pt.find_types(text):
        hit = names & wanted
        if len(hit) == 1:
            found[next(iter(hit))] = (start, pe.trailing_comment_end(text, end), text[start:end])
    return found


def apply_verbatim(sources):
    """Move VERBATIM types. A file takes a header only if the header's other names do not appear in it."""
    text_of = {}
    headers = {}
    for header, names in VERBATIM.items():
        statements = {}
        users = {}
        for rel, text in sources.items():
            blocks = verbatim_blocks(text)
            mine = [n for n in names if n in blocks]
            if not mine:
                continue
            words = set(pe.WORD.findall(pe.without_comments(text)))
            if (set(names) - set(mine)) & words:
                continue
            for name in mine:
                statements.setdefault(name, blocks[name][2])
            users[rel] = mine
        if len(users) < 2:
            continue
        same = all(re.sub(r"\s+", " ", pe.mask(verbatim_blocks(sources[rel])[n][2])) ==
                   re.sub(r"\s+", " ", pe.mask(statements[n])) for rel, mine in users.items() for n in mine)
        if not same:
            continue
        headers[header] = [statements[n] for n in names if n in statements]
        for rel, mine in users.items():
            text = text_of.get(rel, sources[rel])
            for name in mine:
                start, stop, _ = verbatim_blocks(text)[name]
                text = text[:start] + text[stop:]
            text = re.sub(r"\n{3,}", "\n\n", text)
            first = re.search(r"^#include [^\n]*\n(?:#include [^\n]*\n)*", text, re.M)
            at = first.end() if first else 0
            text_of[rel] = text[:at] + f'#include "{header}"\n' + text[at:]
    return headers, text_of


def apply_to_source(text):
    """(new text, {old: new}, headers), or the text unchanged if a replaced type has a provenance note."""
    replacements = find_replacements(text)
    if not replacements:
        return text, {}, set()
    keep = [r for r in replacements if not pe.PROVENANCE.search(text[r[0]:r[1]])]
    parts, last = [], 0
    for start, stop, _, _ in keep:
        parts.append(text[last:start])
        last = stop
    parts.append(text[last:])
    renames = {name: merge["new"] for _, _, name, merge in keep}
    headers = {merge["header"] for _, _, _, merge in keep}
    new = rename_code(re.sub(r"\n{3,}", "\n\n", "".join(parts)), renames)
    includes = "".join(f'#include "{h}"\n' for h in sorted(headers) if f'#include "{h}"' not in new)
    first = re.search(r"^#include [^\n]*\n(?:#include [^\n]*\n)*", new, re.M)
    at = first.end() if first else 0
    return new[:at] + includes + new[at:], renames, headers


def header_text(header, merges):
    guard = re.sub(r"\W", "_", header).upper()
    lines = [f"#ifndef {guard}", f"#define {guard}", "", '#include "common.h"', ""]
    for merge in merges:
        fields = [f"{field.strip()};" for field in merge["definition"].split(";") if field.strip()]
        lines += ["typedef struct {"] + [f"    {field}" for field in fields] + [f"}} {merge['new']};", ""]
    return "\n".join(lines + [f"#endif /* {guard} */", ""])


def fragment_paths(rel):
    stem = rel[len("src/"):-len(".c")]
    return sorted((ROOT / "src/versions").glob(f"*/{stem}/*.c"))


def main():
    write = "--write" in sys.argv[1:]
    counts = collections.Counter()
    files = 0
    for rel in pe.sources(["src"]):
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        new, renames, _ = apply_to_source(text)
        if not renames:
            continue
        files += 1
        counts.update(renames.values())
        if write:
            path.write_text(new, errors="surrogateescape")
            for fragment in fragment_paths(rel):
                old = fragment.read_text(errors="surrogateescape")
                changed = rename_code(old, renames)
                if changed != old:
                    fragment.write_text(changed, errors="surrogateescape")
    sources = {rel: (ROOT / rel).read_text(errors="surrogateescape") for rel in pe.sources(["src"])}
    headers, text_of = apply_verbatim(sources)
    for header, statements in headers.items():
        print(f"{len(statements):4} types: {header}")
    print(f"     verbatim: {len(text_of)} files")
    if write:
        for rel, text in text_of.items():
            (ROOT / rel).write_text(text, errors="surrogateescape")
        for header, statements in headers.items():
            guard = re.sub(r"\W", "_", header).upper()
            target = ROOT / "include" / header
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(f"#ifndef {guard}\n#define {guard}\n\n{VERBATIM_INCLUDES}\n" +
                              "\n\n".join(statements) + f"\n\n#endif /* {guard} */\n")
    if write:
        by_header = collections.defaultdict(list)
        for merge in MERGES:
            # A header that exists keeps every type: other files already use them.
            if counts[merge["new"]] or (ROOT / "include" / merge["header"]).exists():
                by_header[merge["header"]].append(merge)
        for header, merges in by_header.items():
            target = ROOT / "include" / header
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(header_text(header, merges))
    for name, count in counts.most_common():
        print(f"{count:4} files: {name}")
    print(f"{'changed' if write else 'would change'} {files} files")


if __name__ == "__main__":
    main()
