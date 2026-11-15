"""The typed fields of each Ratchet & Clank moby class's pvars, from the decompilation's
moby_pvars.h (or, in a copy without it, from pvar_layouts.json, made when the copy was bundled:
tools/bundle_standalone.py). The level editor's tools show and pack a moby's pvars by them.

    pvar_layouts.py --export-layouts FILE    writes pvar_layouts.json
"""

import json
import re
from pathlib import Path

from formats import FormatError
from mobys import DECOMPILATION

SCALARS = {"f32": "f", "s32": "i", "u32": "I", "s16": "h", "u16": "H", "s8": "b", "u8": "B"}

_layouts = None
PACKAGED_LAYOUTS = Path(__file__).with_name("pvar_layouts.json")


def read_layouts(path: Path) -> dict[int, list[tuple]]:
    """Read the generated field facts carried by an installed editor, without source headers."""
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        raise FormatError(f"Cannot read packaged pvar layouts; update the level tools ({error})") from None
    if not isinstance(document, dict) or document.get("format") != 1 or not isinstance(document.get("classes"), dict):
        raise FormatError("The packaged pvar layouts are incompatible; update the level tools")
    result = {}
    for ident, fields in document["classes"].items():
        if not ident.isdecimal() or not isinstance(fields, list):
            raise FormatError("Invalid class in the packaged pvar layouts")
        known = set()
        checked = []
        for field in fields:
            if (not isinstance(field, list) or len(field) != 4 or not isinstance(field[0], str)
                    or not field[0] or field[0] in known or type(field[1]) is not int or field[1] < 0
                    or not isinstance(field[2], str) or field[2] not in SCALARS
                    or type(field[3]) is not int or field[3] < 0):
                raise FormatError(f"Invalid field in packaged pvar class {ident}")
            known.add(field[0])
            checked.append(tuple(field))
        result[int(ident)] = checked
    return result


def export_layouts(path: Path) -> None:
    """Generate data-only layouts while building the installed tools; no game/SDK bytes are copied."""
    layouts = class_layouts()
    if not layouts:
        raise FormatError("Cannot build the editor package without the qualified pvar layouts")
    data = {"format": 1, "source": "games/rac1/decomp/include/pvars/moby_pvars.h",
            "classes": {str(c): fields for c, fields in sorted(layouts.items())}}
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=1, allow_nan=False) + "\n", encoding="utf-8", newline="\n")


def class_layouts() -> dict[int, list[tuple]]:
    """class -> the typed fields of its pvars (name, offset, scalar type, array length or 0), from
    the decompilation's moby_pvars.h: `struct update<class>`. A struct whose offsets as parsed differ
    from those its comments give is left out (its words are still shown). Installed copies
    read the same field facts generated at bundle time instead of requiring the checkout."""
    global _layouts
    if _layouts is not None:
        return _layouts
    header = DECOMPILATION / "include" / "pvars" / "moby_pvars.h"
    if not header.is_file():
        _layouts = read_layouts(PACKAGED_LAYOUTS) if PACKAGED_LAYOUTS.is_file() else {}
        return _layouts
    _layouts = {}
    from cparse import Types
    types = Types()
    include = DECOMPILATION / "include"
    for path in (include / "common.h", include / "structs" / "misc.h"):  # what moby_pvars.h includes
        if path.is_file():
            types.read(path.read_text(errors="replace"))
    # cparse reads `short int` as two names; the header writes it so.
    text = re.sub(r"\b(short|long)\s+int\b", r"\1", header.read_text(errors="replace"))
    types.read(text)
    for class_id, body in re.findall(r"^struct update(\d+) \{[^\n]*\n(.*?)^\};", text, re.M | re.S):
        s = types.structs.get(f"update{class_id}")
        if s is None or not s.exact:
            continue
        stated = {name: int(at, 16) for at, name in re.findall(r"/\* 0x([0-9a-fA-F]+) \*/[^;]*?\b(\w+)\s*(?:\[[^\]]*\])?;", body)}
        if any(stated.get(f.name, f.offset) != f.offset for f in s.fields):
            continue
        _layouts[int(class_id)] = flatten(types, s, "", 0, 0)
    return _layouts


def flatten(types, s, prefix: str, base: int, depth: int) -> list[tuple]:
    """The named scalar fields of struct S (and of the structs in it), as (name, offset, type, n)."""
    out = []
    for f in s.fields:
        if re.match(r"(unknown|pad|unk)", f.name) or f.ctype.ptr:
            continue
        name, t = f"{prefix}{f.name}", f.ctype
        if t.base in SCALARS and len(t.dims) <= 1 and (not t.dims or t.dims[0] <= 16):
            out.append((name, base + f.offset, t.base, t.dims[0] if t.dims else 0))
        elif t.base in types.structs and depth < 3:
            inner = types.structs[t.base]
            if not inner.exact:
                continue
            count = t.dims[0] if len(t.dims) == 1 and t.dims[0] <= 8 else (1 if not t.dims else 0)
            for k in range(count):
                tag = f"{name}{k}_" if t.dims else f"{name}_"
                out += flatten(types, inner, tag, base + f.offset + k * inner.size, depth + 1)
    return out


def main() -> None:
    import argparse
    parser = argparse.ArgumentParser(description="Generate the typed pvar layouts for installed level tools")
    parser.add_argument("--export-layouts", type=Path, required=True)
    args = parser.parse_args()
    try:
        export_layouts(args.export_layouts)
    except (OSError, ValueError) as error:
        parser.exit(1, f"pvar layouts: {error}\n")


if __name__ == "__main__":
    main()
