#!/usr/bin/env python3
"""
Data in C, with no retail bytes in the repository (LEGAL.md).

  python3 tools/data_in_c.py generate   # write build-sn/data_inc/<label>.inc from your own asm/data/
  python3 tools/data_in_c.py objects    # the data objects, for Makefile.sn
  python3 tools/data_in_c.py check      # every row has its source file, and its source names it
  python3 tools/data_in_c.py add D_00165580:src/game/ui/pause.c ...   # give blocks a file
  python3 tools/data_in_c.py write-out  # write pointer tables out as C, and type float blocks
  python3 tools/data_in_c.py set-type D_001DD568=StashBlock ...      # give blocks a type from config/data_types.tsv
  python3 tools/data_in_c.py rewrite    # write every source file again from the table
  python3 tools/data_in_c.py complete   # give every remaining data label a block, or a place inside one

A file in src/data/ gives a block of retail's data its type, its name and the file that owns it. Its values come from
an include file generated locally from your baserom, so they are never committed:

    u32 fog_table[] DATA_SECTION(D_00165580) = {
    #include "data_inc/D_00165580.inc"
    };

The file is compiled on its own, and code keeps its extern. So moving data here cannot change how code addresses it.
The block's own section goes in the hole cut for it in the data blob (tools/jump_tables.py, tools/split_data_s.py), and
tools/check_image.py proves the bytes are retail's.

A table of only symbols and zeros is written out in its file, because symbol names are not retail's bytes.
A block of only zeros needs no values at all, so it is plain C too.

config/data_in_c.tsv lists the blocks: label, element type (`table` for one written out), source file.
"""
import ast
import math
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import core_rodata  # noqa: E402
import data_types  # noqa: E402
import split_data_s  # noqa: E402
import names_header  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
TABLE = ROOT / "config/data_in_c.tsv"
# Each blob, by segment: its .s file, where it starts, the prefix of its pieces, and their section.
SEGMENTS = {"core": ("core_data.data", 0x0012F580, "core_data", ".data"),
            "core_rdata": ("core_rdata.rodata", 0x00152300, "core_rdata", ".rodata"),
            "core_lit": ("core_lit.rodata", 0x0015ED80, "core_lit", ".rodata"),
            "lit": ("lit.rodata", 0x0015F000, "lit", ".rodata"),
            "data": ("data.data", 0x00165580, "data", ".data"),
            "lvl_vtbl": ("lvl_vtbl.data", 0x001E8F00, "lvl_vtbl", ".data"),
            "lvl_camvtbl": ("lvl_camvtbl.data", 0x001E8F80, "lvl_camvtbl", ".data"),
            "lvl_sndvtbl": ("lvl_sndvtbl.data", 0x001E9000, "lvl_sndvtbl", ".data")}
BLOBS = {name: ROOT / f"asm/data/{row[0]}.s" for name, row in SEGMENTS.items()}
OUT = ROOT / "build-sn/data_inc"
BLOCK = re.compile(r"^dlabel (\S+)\n(.*?)^enddlabel \1", re.S | re.M)
DIRECTIVE = {".word": "u32", ".float": "u32", ".short": "u16", ".byte": "u8"}
# A word that points at a symbol, with an optional offset. Each gets a generated declaration (see references).
SYMBOL = re.compile(r"((?:func|D)_[0-9A-Za-z_]+)(?:\s*\+\s*(0[xX][0-9A-Fa-f]+|[0-9]+))?")
RAW_WORD = re.compile(r"/\*\s*[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+([0-9A-Fa-f]{8})\s*\*/")
PER_LINE = 8
HEADER = "# Blocks of retail's data with a C file (tools/data_in_c.py). No values here.\n# label\ttype\tsource\n"


def rows():
    """[(label, element type, source file)]"""
    if not TABLE.exists():
        return []
    lines = (line.split("\t") for line in TABLE.read_text().splitlines() if line and not line.startswith("#"))
    return [(label, element, source) for label, element, source in lines]


def object_of(source):
    """src/data/game/ui/pause.c -> build-sn/data/game/ui/pause.o"""
    return "build-sn/" + source[len("src/"):-len(".c")] + ".o"


def segment_of(label):
    at = int(label.rsplit("_", 1)[1], 16)
    return max((row[1], name) for name, row in SEGMENTS.items() if row[1] <= at)[1] if at >= 0x0012F580 else "core"


def owned_elsewhere():
    """Labels in core_rdata that compiled objects bring themselves (config/core_rodata.txt)."""
    return {label for _, _, labels in core_rodata.sections() for label in labels}


def provided(segment="data"):
    """(label, object, input section) of every block a data file provides in this blob."""
    return [(label, object_of(source), f".data.{label}") for label, _, source in rows() if segment_of(label) == segment]


def raw_blocks():
    """[(label, body)] of both blobs, in address order within each.

    Data that splat merged into a jump table's block after the table (strings, say) has no label of its own, so it gets
    one from its address. Each line keeps its place with a marker: the .align lines between them are gone.
    """
    out = []
    heads = {row[0] for row in rows()}
    for path in BLOBS.values():
        for label, whole in BLOCK.findall(path.read_text()):
            pieces = split_at_heads(label, whole, heads)
            out.extend(pieces)
            body = pieces[-1][1] if len(pieces) == 1 else whole
            if label.startswith("jtbl_"):
                rest = split_data_s.table_rest(body.splitlines(keepends=True))
                if rest:
                    start = split_data_s.first_address(rest)
                    out.append((f"D_{start:08X}", "".join(f".at 0x{split_data_s.first_address([line]):X}\n{line}" for line in rest)))
    return out


def address(label):
    return int(label.rsplit("_", 1)[1], 16)


def split_at_heads(label, body, heads):
    """[(label, body)]: a block of the disassembly, cut where a row of the table starts inside it.

    The disassembly only has a label where it saw a reference, and level code reaches into this data through fake bases. So
    a variable can have no label of its own.
    """
    pieces, current, name = [], [], label
    for line in body.splitlines(keepends=True):
        at = split_data_s.first_address([line])
        here = f"D_{at:08X}" if at is not None else None
        if here and here != name and here in heads and current:
            pieces.append((name, "".join(current)))
            current, name = [], here
        current.append(line)
    pieces.append((name, "".join(current)))
    return pieces


def group_labels(labels, heads, owned=frozenset()):
    """{head: [labels inside it]}. A label not on 8 bytes is inside the block before it: GCC puts an aggregate on 8, so
    retail had one record there, and the labels are places the code reaches into it."""
    groups, current = {}, None
    for label in labels:
        if label in heads:
            current = label
            groups[label] = []
        elif label in owned:
            current = None
        elif current and label.startswith("D_") and address(label) % 8 and segment_of(label) == segment_of(current):
            groups[current].append(label)
        else:
            current = None
    return groups


def interior():
    """{label inside a block: the block's own label}"""
    heads = {row[0] for row in rows()}
    groups = group_labels([label for label, _ in raw_blocks()], heads, owned_elsewhere())
    return {inner: head for head, inners in groups.items() for inner in inners}


def all_blocks():
    """{label: body} for both blobs. A block's body includes the labels inside it."""
    raw = raw_blocks()
    bodies = dict(raw)
    heads = {row[0] for row in rows()}
    blocks = {}
    groups = group_labels([label for label, _ in raw], heads, owned_elsewhere())
    for label, inners in groups.items():
        blocks[label] = at_marker(label) + bodies[label] + "".join(at_marker(i) + bodies[i] for i in inners)
    inside = {i for inners in groups.values() for i in inners}
    for label, body in raw:
        if label not in blocks and label not in inside:
            blocks[label] = at_marker(label) + body
    return blocks


def at_marker(label):
    """A line that says where a block's next bytes belong, so a merged block keeps its .align lines right."""
    return f".at 0x{address(label):X}\n"


def parse_mixed(body):
    """(u32, words) of any block, built byte by byte. None if a line is not understood."""
    data, symbols, start = bytearray(), {}, None
    for line in body.splitlines():
        raw = RAW_WORD.search(line)
        text = re.sub(r"/\*.*?\*/", " ", line).strip()
        if not text:
            continue
        m = re.fullmatch(r"(\.\w+)\s*(.*)", text)
        if not m:
            continue
        kind, rest = m.groups()
        if kind == ".at" and start is None:
            start = int(rest, 0)
        here = (start or 0) + len(data)
        if kind == ".at":
            target = int(rest, 0)
            if target < here:
                return None
            data += bytes(target - here)
        elif kind == ".align":
            size = 1 << int(rest, 0)
            data += bytes(-here % size)
        elif kind == ".space":
            data += bytes(int(rest.split(",")[0], 0))
        elif kind == ".float":
            if not raw:
                return None
            data += bytes.fromhex(raw.group(1))
        elif kind == ".word":
            for value in rest.split(","):
                value = value.strip()
                symbol = SYMBOL.fullmatch(value)
                if symbol:
                    offset = f" + {symbol.group(2)}" if symbol.group(2) else ""
                    symbols[len(data)] = f"(u32)ref_{symbol.group(1)}{offset}"
                    data += bytes(4)
                elif re.fullmatch(r"-?(0[xX][0-9A-Fa-f]+|[0-9]+)", value):
                    data += (int(value, 0) & 0xFFFFFFFF).to_bytes(4, "little")
                else:
                    return None
        elif kind in (".short", ".byte"):
            width = 2 if kind == ".short" else 1
            for value in rest.split(","):
                value = value.strip()
                if not re.fullmatch(r"-?(0[xX][0-9A-Fa-f]+|[0-9]+)", value):
                    return None
                data += (int(value, 0) & ((1 << (8 * width)) - 1)).to_bytes(width, "little")
        elif kind in (".asciz", ".ascii"):
            try:
                decoded = ast.literal_eval(rest).encode("latin-1", "surrogateescape")
            except (ValueError, SyntaxError, UnicodeError):
                return None
            data += decoded + (b"\0" if kind == ".asciz" else b"")
        else:
            return None
    if len(data) % 4 and not symbols and data:
        return "u8", [f"0x{byte:02X}" for byte in data]     # padding it to a word could run into the next label
    data += bytes(-len(data) % 4)
    if any(at % 4 for at in symbols):
        return None
    words = [symbols.get(at, f"0x{int.from_bytes(data[at:at + 4], 'little'):08X}") for at in range(0, len(data), 4)]
    return ("u32", words) if words else None


def parse(body):
    """(element type, values) of a block: plain if it is all one simple form, else as words. None if neither works."""
    return parse_plain(body) or parse_mixed(body)


def parse_plain(body):
    """(element type, values) of a block, or None if it is not all one simple form.

    A float keeps its exact bits, taken from the disassembly's raw word: a decimal could round, and denormals have
    bitten this build before. Trailing .align lines are padding that the pieces around the hole already lay out.
    """
    if body.count(".at 0x") > 1:
        return None
    element, found = None, []
    for line in body.splitlines():
        raw = RAW_WORD.search(line)
        text = re.sub(r"/\*.*?\*/", " ", line).strip()
        if not text or text.startswith((".align", ".at ")):
            continue
        string = re.fullmatch(r'\.asciz\s+(".*")', text)
        if string:
            if element or found:
                return None
            element, found = "char", [string.group(1)]
            continue
        m = re.fullmatch(r"(\.\w+)\s+(.*)", text)
        if not m or m.group(1) not in DIRECTIVE or element == "char":
            return None
        kind = DIRECTIVE[m.group(1)]
        if element not in (None, kind):
            return None
        element = kind
        if m.group(1) == ".float":
            if not raw:
                return None
            # The raw column is in file order, and the EE is little-endian.
            found.append("0x" + bytes.fromhex(raw.group(1))[::-1].hex().upper())
            continue
        for value in m.group(2).split(","):
            value = value.strip()
            symbol = SYMBOL.fullmatch(value)
            if symbol and m.group(1) == ".word":
                offset = f" + {symbol.group(2)}" if symbol.group(2) else ""
                found.append(f"(u32)ref_{symbol.group(1)}{offset}")
            elif re.fullmatch(r"-?(0[xX][0-9A-Fa-f]+|[0-9]+)", value):
                found.append(value)
            else:
                return None
    return (element, found) if element else None


def references(found):
    """Generated declarations for the symbols a block points at.

    Each is aliased to the real symbol, so it cannot clash with a type the same symbol has in this file. The alias is
    only in generated output: committed C keeps to the no-new-aliases rule.
    """
    names = sorted({m for value in found for m in re.findall(r"\bref_(\w+)", value)})
    return "".join(f'extern char ref_{name}[] __asm__("{name}");\n' for name in names)


def render(found, element="u32"):
    """One value is a scalar's initialiser. More are an array's. A string is its own initialiser."""
    if len(found) == 1:
        return found[0] + "\n"
    return "".join(", ".join(found[i:i + PER_LINE]) + ",\n" for i in range(0, len(found), PER_LINE))


def fits_retail(label, count, element="u32"):
    """GCC 2.95 aligns an aggregate to 8, so a longer block or a string not on 8 was something else in retail."""
    return (count == 1 and element != "char") or int(label.rsplit("_", 1)[1], 16) % 8 == 0


def generate():
    blocks = all_blocks()
    types = data_types.load()
    OUT.mkdir(parents=True, exist_ok=True)
    for stale in [*OUT.glob("*.inc"), *OUT.glob("*.refs")]:
        stale.unlink()
    for label, element, _ in rows():
        parsed = parse(blocks[label]) if label in blocks else None
        if parsed is None or (parsed[0] != element and element not in ("table", "float") and element not in types):
            sys.exit(f"*** {label}: its block is not plain {element}")
        if element == "table" or (element != "char" and is_zero(parsed[1])):
            continue
        if element in types:
            values = render_typed(parsed[1], types[element][2], types[element][1])
        else:
            values = [float_literal(word) for word in parsed[1]] if element == "float" else parsed[1]
        (OUT / f"{label}.inc").write_text(render(values, element))
        (OUT / f"{label}.refs").write_text(references(parsed[1]))
    print(f"wrote {len(rows())} include files to {OUT.relative_to(ROOT)}")


def next_piece_fits(label, blocks):
    """The blob's piece after a cut starts at the next block, and the assembler aligns a .data section to at least 4."""
    labels = list(blocks)
    at = labels.index(label)
    if at + 1 == len(labels):
        return True
    return int(labels[at + 1].rsplit("_", 1)[1], 16) % 4 == 0


def add(pairs):
    """Add blocks: each pair is LABEL:OWNER (a source file under src/, or shared for a block with no one owner)."""
    blocks = all_blocks()
    table = rows()
    known = {row[0] for row in table}
    names = readable_names()
    for pair in pairs:
        label, owner = pair.split(":", 1)
        if label in known:
            continue
        if not label.startswith("D_"):
            sys.exit(f"*** {label}: not a data block. A jump table comes from its compiled function")
        parsed = parse(blocks[label]) if label in blocks else None
        if parsed is None:
            sys.exit(f"*** {label}: not a plain block")
        if not fits_retail(label, len(parsed[1]), parsed[0]):
            sys.exit(f"*** {label}: not on an 8-byte boundary, so retail had something else here")
        if not next_piece_fits(label, blocks):
            sys.exit(f"*** {label}: the block after it is not on a 4-byte boundary, so the blob cannot be cut here")
        source = "src/data/shared/data.c" if owner == "shared" else "src/data/" + owner[len("src/"):]
        table.append((label, parsed[0], source))
    write_all(table, blocks, names)
    print(f"{len(table)} blocks in {len({row[2] for row in table})} files")


def write_all(table, blocks, names):
    write_rows(table)
    for source in sorted({row[2] for row in table}):
        entries = []
        for row in table:
            if row[2] == source:
                found = parse(blocks[row[0]])[1]
                entries.append((row[0], row[1], len(found), bool(references(found))))
        write_source(source, sorted(entries), names, blocks)


def alignment_allows(label, words, size):
    """A block not on 8 bytes can only be one whole record: GCC would put an array of them on 8."""
    return address(label) % 8 == 0 or words * 4 == size


def set_types(pairs):
    """LABEL=TYPE gives a block a type from config/data_types.tsv. One that does not fit stays as it is, and says why."""
    blocks = all_blocks()
    types = data_types.load()
    wanted = dict(pair.split("=", 1) for pair in pairs)
    table = []
    for label, element, source in rows():
        name = wanted.get(label)
        if name and element == "u32":
            try:
                if not alignment_allows(label, len(parse(blocks[label])[1]), types[name][1]):
                    raise ValueError("it is not on 8 bytes, and it is more than one record")
                render_typed(parse(blocks[label])[1], types[name][2], types[name][1])
                element = name
            except ValueError as problem:
                print(f"{label}: stays {element}: {problem}")
        table.append((label, element, source))
    write_all(table, blocks, readable_names())


def inner_symbols(label):
    """Linker lines that keep the labels inside a block defined: the code reaches them by name."""
    inside = interior()
    return [f"    {inner} = {label} + 0x{address(inner) - address(label):X};" for inner in sorted(inside, key=address)
            if inside[inner] == label]


def single_value(body):
    """True if a block is one number, so it can be a variable of its own at any aligned address."""
    parsed = parse_plain(at_marker("D_00000000") + body)
    return bool(parsed and len(parsed[1]) == 1 and parsed[0] != "char")


def starts_block(label, body, next_starts):
    """Whether a label starts its own block: on 8 bytes, or one number with nothing inside it after."""
    return address(label) % 8 == 0 or (single_value(body) and next_starts)


def complete():
    """Give every remaining data label a place: its own block if it can start one, else inside the block before it."""
    raw = [pair for pair in raw_blocks() if pair[0] not in owned_elsewhere()]
    table = {row[0]: row for row in rows()}
    next_starts = True
    starts = set()
    for at in range(len(raw) - 1, -1, -1):
        label, body = raw[at]
        if not label.startswith("D_"):
            next_starts = True
            continue
        if at + 1 < len(raw) and segment_of(raw[at + 1][0]) != segment_of(label):
            next_starts = True
        begins = label in table or starts_block(label, body, next_starts)
        if begins:
            starts.add(label)
        next_starts = begins
    orphans = []
    current = None
    for label, _ in raw:
        if not label.startswith("D_"):
            current = None
        elif label in starts:
            table.setdefault(label, (label, "u32", "src/data/shared/data.c"))
            current = label
        elif current is None or segment_of(current) != segment_of(label):
            orphans.append(label)
    write_rows(list(table.values()))
    blocks = all_blocks()
    types = data_types.load()
    final = []
    for label, element, source in rows():
        parsed = parse(blocks[label])
        if parsed is None or (address(label) % 8 and len(parsed[1]) > 1 and parsed[0] != "char"):
            orphans.append(label)
            continue
        if element in ("u32", "u16", "u8", "char") and parsed[0] != element:
            element = parsed[0]
        if parsed[0] != "u32" and element not in ("u32", "u16", "u8", "char"):
            element = parsed[0]                     # a real type is written from words: this block is not words
        if element == "table" and not is_table(parsed[1]):
            element = parsed[0]
        if element == "float" and not all(float_literal(word) for word in parsed[1]):
            element = "u32"
        if element in types:
            try:
                render_typed(parsed[1], types[element][2], types[element][1])
            except ValueError:
                element = parsed[0]
        final.append((label, element, source))
    write_all(final, all_blocks(), readable_names())
    print(f"{len(final)} blocks, {len(interior())} labels inside them; left in the blob: {orphans}")


def write_rows(table):
    TABLE.write_text(HEADER + "".join("\t".join(row) + "\n" for row in sorted(table)))


def write_out():
    """Write pointer tables out as C, and give float blocks their type. Other values stay generated."""
    blocks = all_blocks()
    table = []
    tables = floats = 0
    for label, element, source in rows():
        found = parse(blocks[label])[1]
        if element == "u32" and is_table(found):
            element, tables = "table", tables + 1
        elif element == "u32" and is_float_block(blocks[label], found):
            element, floats = "float", floats + 1
        table.append((label, element, source))
    write_all(table, blocks, readable_names())
    print(f"{tables} tables written out, {floats} blocks are float")


def readable_names():
    """{address name: readable name}, from the name table."""
    return {address: name for name, address in re.findall(r"(?m)^#define\s+(\w+)\s+((?:D|func)_\w+)\s*$",
                                                          names_header.read(ROOT / "include/names.h"))}


def is_table(found):
    """True if a block is only symbols and zeros. Its names are not retail's bytes, so it can be written out."""
    pointers = [value for value in found if "ref_" in value]
    return bool(pointers) and all("ref_" in value or int(value, 0) == 0 for value in found)


def float_literal(word):
    """A C literal for a word holding a float, or None if the compiler cannot be trusted to read it back."""
    value = struct.unpack("<f", int(word, 16).to_bytes(4, "little"))[0]
    if not math.isfinite(value) or (value != 0 and abs(value) < sys.float_info.min) or abs(value) < 1.2e-38 and value:
        return None
    if value == 0 and word.upper() != "0X00000000" and word.upper() != "0":
        return None
    text = f"{value:.9e}f"
    return text if struct.pack("<f", float(text[:-1])) == int(word, 16).to_bytes(4, "little") else None


def is_float_block(body, found):
    """True if a block is written with .float only, and each of its values can be a literal."""
    kinds = set(re.findall(r"^\s*(?:/\*.*?\*/\s*)?\.(\w+)", body, re.M)) - {"at"}
    return kinds == {"float"} and all(float_literal(word) for word in found)


def is_zero(found):
    """True if a block is all zeros. It holds no retail numbers, so it needs no generated values."""
    return all("ref_" not in value and int(value, 0) == 0 for value in found)


def render_typed(found, fields, size):
    """A flat initialiser for words of data, one value per field. Raises ValueError if the words cannot be that type."""
    words = len(found) * 4
    tail = words % size if size else 0
    if not size or tail >= 8:
        raise ValueError(f"{words} bytes is not a whole number of {size}-byte records")
    data, symbols = bytearray(), {}
    for at, value in enumerate(found):
        if "ref_" in value:
            symbols[at * 4] = value[len("(u32)"):]
            data += bytes(4)
        else:
            data += (int(value, 0) & 0xFFFFFFFF).to_bytes(4, "little")
    if any(data[words - tail:]) or any(at >= words - tail for at in symbols):
        raise ValueError(f"the {tail} bytes after the last record are not zero, so they are not alignment")
    out = []
    for record in range(0, words - tail, size):
        covered = set()
        for offset, kind, width in fields:
            at = record + offset
            covered.update(range(at, at + width))
            raw = bytes(data[at:at + width])
            if at in symbols:
                if width != 4 or kind not in "ip":
                    raise ValueError(f"a symbol at {at:#x} in a {kind}{width} field")
                out.append(f"(void *){symbols[at]}" if kind == "p" else f"(u32){symbols[at]}")
            elif any(at < s < at + width for s in symbols):
                raise ValueError(f"a symbol inside the field at {at:#x}")
            elif kind == "q":
                if any(raw):
                    raise ValueError(f"a non-zero quad at {at:#x}")
                out.append("0")
            elif kind == "f":
                literal = float_literal(f"0x{int.from_bytes(raw, 'little'):08X}")
                if literal is None:
                    raise ValueError(f"no literal for the float at {at:#x}")
                out.append(literal)
            else:
                out.append(f"0x{int.from_bytes(raw, 'little'):X}")
        for at in range(record, record + size):
            if at not in covered and (data[at] or at in symbols):
                raise ValueError(f"padding at {at:#x} is not zero")
    return out


def table_declarations(found, names, here):
    """Declarations for the symbols of a table. here: {name: (element, count)} of the blocks in the same file."""
    lines = []
    for symbol in sorted({m for value in found for m in re.findall(r"\bref_(\w+)", value)}):
        name = names.get(symbol, symbol)
        if name in here:
            element, count = here[name]
            element = "void *" if element == "table" else element + " "
            lines.append(f"extern {element}{name}{'[]' if count > 1 or element.startswith('char') else ''};\n")
        elif symbol.startswith("func_"):
            lines.append(f"extern void {name}();\n")
        else:
            lines.append(f"extern char {name}[];\n")
    return lines


def table_values(found, names, here):
    """Each value of a table, in C."""
    out = []
    for value in found:
        m = re.fullmatch(r"\(u32\)ref_(\w+)(?: \+ (\w+))?", value)
        if not m:
            out.append("0")
            continue
        name = names.get(m.group(1), m.group(1))
        scalar = name in here and here[name][1] == 1 and here[name][0] != "char"
        text = f"&{name}" if scalar else name
        out.append(f"(char *){text} + {m.group(2)}" if m.group(2) else text)
    return out


def table_text(label, found, names, here):
    """A table's definition, written out: symbol names only."""
    name = names.get(label, label)
    values = table_values(found, names, here)
    if len(values) == 1:
        return f"void *{name} DATA_SECTION({label}) = {values[0]};\n\n"
    return f"void *{name}[] DATA_SECTION({label}) = {{\n" + "".join(f"    {v},\n" for v in values) + "};\n\n"


def write_source(source, entries, names, blocks=None):
    """entries: (label, element type, number of values, whether it points at symbols)."""
    if source == "src/data/shared/data.c":
        about = "Data with no one owner: several files use it, or no C file does yet."
    else:
        about = f"Data owned by {'src/' + source[len('src/data/'):]}."
    parts = [f"/* {about} Its values come from your own baserom (tools/data_in_c.py). */\n", '#include "common.h"\n\n']
    types = data_types.load()
    headers = sorted({types[e[1]][0] for e in entries if e[1] in types and types[e[1]][0]})
    parts.extend(f'#include "{header}"\n' for header in headers)
    if headers:
        parts.append("\n")
    here = {names.get(e[0], e[0]): (e[1], e[2]) for e in entries}
    tables = [e for e in entries if e[1] == "table"]
    declared = []
    for label, *_ in tables:
        for line in table_declarations(parse(blocks[label])[1], names, here):
            if line not in declared:
                declared.append(line)
    # A forward declaration of a block in this file comes after its own definition's type is known: same text.
    parts.extend(declared)
    if declared:
        parts.append("\n")
    for label, element, count, pointers in entries:
        include = f'#include "data_inc/{label}.inc"\n'
        name = names.get(label, label)
        if element == "table":
            parts.append(table_text(label, parse(blocks[label])[1], names, here))
            continue
        typed = types.get(element)
        if element != "char" and blocks and is_zero(parse(blocks[label])[1]):
            length = count * 4 // typed[1] if typed else count
            shape = "" if length == 1 and (not typed or count * 4 == typed[1]) else f"[{length}]"
            zero = "{0}" if shape or typed else "0"
            parts.append(f"{element} {name}{shape} DATA_SECTION({label}) = {zero};\n\n")
            continue
        if typed and count * 4 == typed[1]:
            if pointers:
                parts.append(f'#include "data_inc/{label}.refs"\n')
            parts.append(f"{element} {name} DATA_SECTION({label}) = {{\n{include}}};\n\n")
            continue
        if pointers:
            parts.append(f'#include "data_inc/{label}.refs"\n')
        if element == "char":
            parts.append(f"char {name}[] DATA_SECTION({label}) =\n{include};\n\n")
        elif count == 1 and not typed:
            parts.append(f"{element} {name} DATA_SECTION({label}) =\n{include};\n\n")
        else:
            parts.append(f"{element} {name}[] DATA_SECTION({label}) = {{\n{include}}};\n\n")
    path = ROOT / source
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(parts).rstrip("\n") + "\n")


def check():
    bad = []
    for label, element, source in rows():
        path = ROOT / source
        if not path.exists():
            bad.append(f"{source} is missing")
        elif f"DATA_SECTION({label})" not in path.read_text():
            bad.append(f"{source} does not define {label}")
    if bad:
        sys.exit("*** " + "\n*** ".join(bad))
    print(f"{len(rows())} blocks of data in C")


def main():
    command = (sys.argv[1:] or [""])[0]
    if command == "generate":
        generate()
    elif command == "objects":
        print(" ".join(sorted({object_of(source) for _, _, source in rows()})))
    elif command == "check":
        check()
    elif command == "write-out":
        write_out()
    elif command == "set-type":
        set_types(sys.argv[2:])
    elif command == "rewrite":
        write_all(rows(), all_blocks(), readable_names())
    elif command == "add":
        add(sys.argv[2:])
    elif command == "labels":
        inside = [f"+{inner}" for inner in interior() if segment_of(inner) == sys.argv[2]]
        print(" ".join([row[0] for row in sorted(provided(sys.argv[2]))] + inside))
    elif command == "complete":
        complete()
    elif command == "ld":
        cuts = sorted(provided(sys.argv[2]))
        _, _, prefix, section_name = SEGMENTS[sys.argv[2]]
        for n, (label, obj, section) in enumerate(cuts, 1):
            print(f"    build-sn/{prefix}_{n}.o({section_name})")
            print(f"    {obj}({section})")
            for line in inner_symbols(label):
                print(line)
        print(f"    build-sn/{prefix}_{len(cuts) + 1}.o({section_name})")
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
