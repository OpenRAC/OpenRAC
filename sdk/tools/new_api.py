#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""Add a new call to the end of openrac_api, with every place it needs, in one go.

    python3 sdk/tools/new_api.py world moby_glow --returns int \\
        --params "openrac_mod* mod, uint32_t moby, float strength" \\
        --about "Makes a moby glow ... Returns OPENRAC_OK, or OPENRAC_E_UNKNOWN for a moby that is gone."

It writes the parts that always look the same (sdk/docs/adding-a-call.md says why each is there):

    sdk/include/openrac/openrac_mod.h          the member, last in openrac_api, "Since 1.<next>."
    sdk/api/members.tsv                        its row: area, 1.<next>, experimental (or --stable)
    port/game/common/mod_api/internal.h        the declaration of api_<name>
    port/game/common/mod_api/<area>.cpp        api_<name>, a stub that does nothing yet
    port/game/common/mods.cpp                  g_api.<name> = api_<name>;

Then it prints what is left by hand: the stub's body, the C++ wrapper in mod.hpp, a test, and
bump_api.py minor. Several calls for one minor are added one after the other before that bump.
Stdlib only.
"""
from __future__ import annotations

import argparse
import re
import sys
import textwrap
from pathlib import Path

import gen_api_lock

ROOT = Path(__file__).resolve().parents[2]
HEADER = Path("sdk/include/openrac/openrac_mod.h")
MEMBERS = Path("sdk/api/members.tsv")
INTERNAL = Path("port/game/common/mod_api/internal.h")
AREA_FILE = "port/game/common/mod_api/{area}.cpp"
MODS = Path("port/game/common/mods.cpp")
NAMESPACE_END = "}  // namespace openrac::mods::detail"
FILL_END = "    g_api_without_native_vulkan = g_api;"
WIDTH = 100


def read(root: Path, path: Path) -> str:
    return (root / path).read_text()


def header_member(name: str, returns: str, params: str, about: str, since: str) -> str:
    """The member as the header writes them: a comment, then the pointer, on one line when it fits."""
    comment = textwrap.wrap(f"{about.strip().rstrip('.')}. Since {since}.", WIDTH - 7,
                            break_long_words=False, break_on_hyphens=False)
    lines = ["    /* " + comment[0]] + ["     * " + line for line in comment[1:]]
    lines[-1] += " */"
    one = f"    {returns} (*{name})({params});"
    if len(one) <= WIDTH:
        return "\n".join(lines + [one])
    args = [a.strip() for a in params.split(",")]
    return "\n".join(lines + [f"    {returns} (*{name})(", *(f"        {a}," for a in args[:-1]),
                              f"        {args[-1]}", "    );"])


def stub_body(returns: str) -> str:
    r = returns.strip()
    if r == "void":
        return ""
    if r.endswith("*"):
        return "    return nullptr;\n"
    if r == "int":
        return "    return OPENRAC_E_UNKNOWN;\n"
    return "    return {};\n"


def param_names(params: str) -> list[str]:
    return [re.split(r"[\s*]+", p.strip())[-1] for p in params.split(",") if p.strip() not in ("", "void")]


def cpp_type(returns: str) -> str:
    """uint32_t and the like as the port's C++ writes them."""
    return re.sub(r"\b(u?int(?:8|16|32|64)_t)\b", r"std::\1", returns)


def insert_before_last(text: str, anchor: str, add: str, where: Path) -> str:
    at = text.rfind(anchor)
    if at < 0:
        raise SystemExit(f"{where}: no {anchor!r}")
    return text[:at] + add + text[at:]


def add_call(root: Path, area: str, name: str, returns: str, params: str, about: str,
             experimental: bool) -> str:
    if area not in gen_api_lock.AREAS:
        raise SystemExit(f"no area {area!r}; the areas are {', '.join(gen_api_lock.AREAS)}")
    if not re.fullmatch(r"[a-z][a-z0-9_]*", name):
        raise SystemExit(f"{name!r}: a member's name is lower case words joined by _")
    if not params.strip().startswith("openrac_mod* mod"):
        raise SystemExit("--params must start with 'openrac_mod* mod': every call says which mod asks")
    header = read(root, HEADER)
    members = read(root, MEMBERS)
    if any(line.split("\t")[0] == name for line in members.splitlines()):
        raise SystemExit(f"{MEMBERS}: {name} is already a member")
    area_path = Path(AREA_FILE.format(area=area))
    if not (root / area_path).exists():
        raise SystemExit(f"{area_path}: no such file")
    minor = int(re.search(r"^#define OPENRAC_MOD_API_MINOR (\d+)$", header, re.M).group(1))
    since = f"1.{minor + 1}"
    params = ", ".join(" ".join(p.split()) for p in params.split(","))

    end = header.index("} openrac_api;")
    (root / HEADER).write_text(header[:end] + "\n" + header_member(name, returns, params, about, since) + "\n"
                               + header[end:])
    status = "experimental" if experimental else "stable"
    (root / MEMBERS).write_text(members.rstrip("\n") + f"\n{name}\t{area}\t{since}\t{status}\n")

    signature = f"{cpp_type(returns)} api_{name}({cpp_type(params)})"
    internal = read(root, INTERNAL)
    (root / INTERNAL).write_text(insert_before_last(internal, NAMESPACE_END, f"{signature};\n\n", INTERNAL)
                                 .replace(";\n\n\n" + NAMESPACE_END, ";\n\n" + NAMESPACE_END))
    stub = (f"// openrac_api.{name}. Since {since}.\n{signature} {{\n"
            f"    // TODO: the call's work; until then it does nothing.\n"
            + "".join(f"    static_cast<void>({n});\n" for n in param_names(params))
            + f"{stub_body(returns)}}}\n\n")
    (root / area_path).write_text(insert_before_last(read(root, area_path), NAMESPACE_END, stub, area_path))
    (root / MODS).write_text(insert_before_last(read(root, MODS), FILL_END, f"    g_api.{name} = api_{name};\n",
                                                MODS))
    return since


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("area", help="the member's area: " + ", ".join(gen_api_lock.AREAS))
    ap.add_argument("name", help="the member's name, lower case words joined by _")
    ap.add_argument("--returns", default="int", help="its C return type (default int: OPENRAC_OK or an error)")
    ap.add_argument("--params", required=True, help='its C parameters, first "openrac_mod* mod"')
    ap.add_argument("--about", required=True, help="what it does and returns, for the header's comment")
    ap.add_argument("--stable", action="store_true",
                    help="stable from the start (a new call is experimental until a release has used it)")
    ap.add_argument("--root", type=Path, default=ROOT, help=argparse.SUPPRESS)
    args = ap.parse_args(argv)
    since = add_call(args.root, args.area, args.name, args.returns, args.params, args.about, not args.stable)
    print(textwrap.dedent(f"""\
        openrac_api.{args.name} added (since {since}, area {args.area}). Left by hand:
          1. port/game/common/mod_api/{args.area}.cpp: the body of api_{args.name}
          2. sdk/include/openrac/mod.hpp: its C++ wrapper (an older port's table is shorter: a has_<name>() that checks m_api->size)
          3. a test in the area's port/tests/mod_*_test.cpp, else mod_loader_test.cpp (sdk/docs/adding-a-call.md)
          4. python3 sdk/tools/bump_api.py minor --note "what it does ({args.name})"
        """))
    return 0


if __name__ == "__main__":
    sys.exit(main())
