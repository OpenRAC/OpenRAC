#!/usr/bin/env python3
"""
Move function prototypes that several files declare the same way into one header per source file.

  python3 tools/tidy/move_protos.py            # dry run: show counts
  python3 tools/tidy/move_protos.py --write

A function defined in src/game/render/draw.c goes in include/game/draw.h. A function with no C definition goes in
include/asm_functions/<part>.h, and data goes in include/data/<part>.h, where <part> is the file of include/names/ its
name is in. A declaration moves only if:
  - every file declares the same type, and two or more files do. Spelling does not count: s32 is int, and
    parameter names are ignored. () and (void) are different, because a prototype can change a call.
  - it has no __asm__, SDATA, MACRO_ADDR or NOT_SDA: those steer how the file addresses a symbol
  - it uses only built-in types, so the header needs nothing but common.h
  - at most one file defines it

A file takes a header only if none of that header's other names already appear in the file or its version
fragments. Otherwise a new prototype could clash with a local type, or change a call that had no prototype.
That file keeps its own declarations.

Run tools/check_all.sh after: only a build can prove the bytes did not change.
"""
import collections
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
QUALIFIERS = re.compile(r"__asm__|__attribute__|MACRO_ADDR|NOT_SDA|SDATA")
# struct, union and enum tags, and capitalised typedefs, need a header of their own. Built-in names do not.
NEEDS_TYPES = re.compile(r"\b(?:struct|union|enum)\b|\b[A-Z]\w*\b")
DEFINITION = re.compile(r"^(?!extern\b|typedef\b|static\b)[A-Za-z_][^;{}=\n]*?\b(\w+)\s*\([^;{}]*\)\s*\{", re.M)
MIN_FILES = 2


SPELLINGS = [(r"\bs8\b", "signed char"), (r"\bs16\b", "short"), (r"\bs32\b", "int"), (r"\bs64\b", "long"),
             (r"\bu8\b", "unsigned char"), (r"\bu16\b", "unsigned short"), (r"\bu32\b", "unsigned int"),
             (r"\bu64\b", "unsigned long"), (r"\bf32\b", "float"), (r"\bf64\b", "double"),
             (r"\bshort int\b", "short"), (r"\blong int\b", "long"), (r"\bunsigned\b(?! (?:char|short|int|long))", "unsigned int"),
             (r"\bsigned int\b", "int")]
TYPE_WORDS = {"void", "char", "short", "int", "long", "float", "double", "unsigned", "signed", "const", "volatile"}


def normal(statement):
    return " ".join(statement.split())


def canonical(name, text):
    """The declaration with its name, spelling and parameter names gone: equal means the same type."""
    body = text.replace(name, "@", 1)
    for pattern, spelling in SPELLINGS:
        body = re.sub(pattern, spelling, body)
    m = re.search(r"@\s*\((.*)\)\s*;$", body)
    if m:
        params = []
        for param in pe.split_declarators(m.group(1)):
            words = re.findall(r"\w+|\*+|\[[^\]]*\]|\.\.\.", param)
            # A last word that is not a type word is the parameter's name.
            while words and re.fullmatch(r"\[[^\]]*\]", words[-1]):
                words.pop()
                words.insert(0, "*")
            if len(words) > 1 and re.fullmatch(r"\w+", words[-1]) and words[-1] not in TYPE_WORDS:
                words.pop()
            params.append(" ".join(sorted(w for w in words if w == "*") + [w for w in words if w != "*"]))
        body = body[:m.start(1)] + ",".join(params) + body[m.end(1):]
    return re.sub(r"\s+", " ", body).strip()


def is_candidate(name, text):
    """Is this declaration text one that may move?"""
    if QUALIFIERS.search(text):
        return False
    return not NEEDS_TYPES.search(text.replace(name, " "))


def is_function(name, text):
    return bool(re.search(re.escape(name) + r"\s*\(", text))


def header_of(source):
    """src/game/render/draw.c -> game/draw.h"""
    return source[len("src/"):-len(".c")] + ".h"


def part_of(names):
    """The include/names/ part for a symbol, from whichever of its names has the address in it."""
    for name in sorted(names):
        m = re.match(r"(?:func|D|jtbl)_L(\d\d)_", name)
        if m:
            return "shared_levels" if m.group(1) == "00" else f"level_{m.group(1)}"
    return "executable"


def home_of(name, text, homes, aliases):
    """The header for this declaration, or None if more than one file defines it."""
    if len(homes) > 1:
        return None
    if homes:
        return header_of(next(iter(homes)))
    folder = "asm_functions" if is_function(name, text) else "data"
    return f"{folder}/{part_of({name} | aliases.get(name, set()))}.h"


def plan(sources, fragments, aliases, existing=None):
    """
    sources: {file: text}, fragments: {file: version fragment text}, aliases: {name: {other names}},
    existing: {header: names it already declares}.
    Returns {header: {name: declaration}} and {file: {header: {name}}}: what to write and who takes what.
    """
    # One symbol can be spelled two ways (readable and address name), so group by the symbol.
    def symbol(name):
        return min({name} | aliases.get(name, set()))

    declared = collections.defaultdict(lambda: collections.defaultdict(set))
    defined = collections.defaultdict(set)
    for rel, text in sources.items():
        for name in DEFINITION.findall(pe.COMMENT_OR_STRING.sub(pe.blank, text)):
            defined[name].add(rel)
        for _, _, statement in pe.find_externs(text):
            names = pe.declared_names(statement)
            if names and len(names) == 1:
                text = normal(statement)
                declared[symbol(names[0])][canonical(names[0], text)].add((rel, names[0], text))

    headers = collections.defaultdict(dict)
    users = {}
    for types in declared.values():
        if len(types) != 1:
            continue
        rows = sorted(next(iter(types.values())))
        files = {row[0] for row in rows}
        written = {row[1] for row in rows}
        # Two spellings of the name in one header would redefine nothing, but keep it simple.
        if len(files) < MIN_FILES or len(written) != 1:
            continue
        _, name, text = rows[0]
        if not all(is_candidate(name, row[2]) for row in rows):
            continue
        homes = set(defined.get(name, ()))
        for other in aliases.get(name, ()):
            homes |= defined.get(other, set())
        header = home_of(name, text, homes, aliases)
        if header is None:
            continue
        headers[header][name] = text
        users[name] = files

    takes = collections.defaultdict(dict)
    for header, names in headers.items():
        for rel in sorted({f for n in names for f in users[n]}):
            mine = {n for n in names if rel in users[n]}
            words = set(pe.WORD.findall(pe.without_comments(sources[rel] + "\n" + fragments.get(rel, ""))))
            others = (set(names) | (existing or {}).get(header, set())) - mine
            for other in list(others):
                others |= aliases.get(other, set())
            if not others & words:
                takes[rel][header] = mine

    # A prototype only earns its place in a header if two files really take it.
    counts = collections.Counter(n for per_file in takes.values() for mine in per_file.values() for n in mine)
    final = collections.defaultdict(dict)
    for header, names in headers.items():
        for name, text in names.items():
            if counts[name] >= MIN_FILES:
                final[header][name] = text
    result = collections.defaultdict(dict)
    for rel, per_file in takes.items():
        for header, mine in per_file.items():
            kept = {n for n in mine if n in final[header]}
            if kept:
                result[rel][header] = kept
    return {h: n for h, n in final.items() if n}, dict(result)


def header_text(header, names):
    guard = re.sub(r"\W", "_", header).upper()
    if header.startswith("data/"):
        about = f"Data declared the same way by several files: {header[5:-2]}."
    elif header.startswith("asm_functions/"):
        about = f"Functions with no C definition: {header[len('asm_functions/'):-2]}."
    else:
        about = f"Functions defined in src/{header[:-2]}.c."
    lines = [f"#ifndef {guard}", f"#define {guard}", "", f"/* {about} */", "", '#include "common.h"', ""]
    lines += [names[name] for name in sorted(names)]
    return "\n".join(lines + ["", f"#endif /* {guard} */", ""])


def merge_header(path, header, names):
    """Add these names to a header that may already exist."""
    if path.exists():
        for _, _, statement in pe.find_externs(path.read_text()):
            found = pe.declared_names(statement)
            if found and found[0] not in names:
                names[found[0]] = normal(statement)
    return header_text(header, names)


def apply_to_source(text, header_names):
    """The file with these declarations removed and these headers included."""
    names = {n for mine in header_names.values() for n in mine}
    spans = []
    for start, end, statement in pe.find_externs(text):
        found = pe.declared_names(statement)
        if not found or len(found) != 1 or found[0] not in names:
            continue
        stop = pe.trailing_comment_end(text, end)
        if not pe.PROVENANCE.search(text[start:stop]):
            spans.append((start, stop))
    parts, last = [], 0
    for start, stop in spans:
        parts.append(text[last:start])
        last = stop
    parts.append(text[last:])
    new = re.sub(r"\n{3,}", "\n\n", re.sub(r"^[ \t]+$", "", "".join(parts), flags=re.M))
    includes = "".join(f'#include "{h}"\n' for h in sorted(header_names) if f'#include "{h}"' not in new)
    # After the block of includes at the top. A file can have a later include below its first calls.
    first = re.search(r"^#include [^\n]*\n(?:#include [^\n]*\n)*", new, re.M)
    at = first.end() if first else 0
    return new[:at] + includes + new[at:]


def main():
    write = "--write" in sys.argv[1:]
    aliases = pe.load_aliases()
    files = pe.sources(["src"])
    sources = {rel: (ROOT / rel).read_text(errors="surrogateescape") for rel in files}
    fragments = {rel: pe.fragments_text(rel) for rel in files}
    existing = {}
    for path in (ROOT / "include").rglob("*.h"):
        found = {n for _, _, s in pe.find_externs(path.read_text(errors="replace")) for n in pe.declared_names(s) or []}
        if found:
            existing[path.relative_to(ROOT / "include").as_posix()] = found
    headers, takes = plan(sources, fragments, aliases, existing)
    moved = sum(len(names) for names in headers.values())
    lines = sum(len(mine) for per_file in takes.values() for mine in per_file.values())
    print(f"{moved} prototypes into {len(headers)} headers, used by {len(takes)} files: {lines} declarations replaced")
    if not write:
        return
    for header, names in headers.items():
        path = ROOT / "include" / header
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(merge_header(path, header, names))
    for rel, header_names in takes.items():
        (ROOT / rel).write_text(apply_to_source(sources[rel], header_names), errors="surrogateescape")


if __name__ == "__main__":
    main()
