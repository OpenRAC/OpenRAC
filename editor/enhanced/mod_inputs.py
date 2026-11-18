# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Prepare lighting inputs from the fixed and virtual sectors of an assembled RAC1 stack."""
import json
import struct

from collision import decode
from disc import Disc, LEVEL_HEADER_SIZE, LEVEL_RANGES, SECTOR
from formats import FormatError, span
from godot import collision_glb
from level import SECTION_NAMES, decoded
from mobys import moby_instances


def sector_files(root, name):
    path = root / name
    if not path.is_file():
        return []
    rows = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#"):
            first, count, filename, *_ = line.split("\t")
            rows.append((int(first), int(count), root / filename))
    return rows


def overlaps(first, count, rows):
    return any(start < first + count and first < start + length for start, length, _ in rows)


def read_sectors(disc, first, count, fixed, virtual):
    def read_file(path, offset, size):
        with path.open("rb") as file:
            file.seek(offset * SECTOR)
            data = file.read(size * SECTOR)
        if len(data) != size * SECTOR:
            raise FormatError(f"short lighting input sector read from {path}")
        return data

    for start, length, path in virtual:
        if start <= first and first + count <= start + length:
            return read_file(path, first - start, count)

    data = bytearray(disc.sectors(first, count))
    for start, length, path in fixed:
        lo, hi = max(start, first), min(start + length, first + count)
        if lo < hi:
            data[(lo - first) * SECTOR:(hi - first) * SECTOR] = read_file(path, lo - start, hi - lo)
    return bytes(data)


def prepare_mount(disc, levels, root, fixed, virtual):
    for info in levels:
        level = root / f"level_{info['id']:02}"
        if not level.is_dir():
            continue
        header = read_sectors(disc, info["header_lba"], -(-LEVEL_HEADER_SIZE // SECTOR), fixed, virtual)

        def edited_range(name):
            first, count = struct.unpack_from("<II", header, 8 + LEVEL_RANGES.index(name) * 8)
            original = info["ranges"][name]
            if (first, count) != (original["lba"], original["sectors"]) or overlaps(first, count, fixed):
                return read_sectors(disc, first, count, fixed, virtual)
            return None

        gameplay = edited_range(disc.version.gameplay)
        data = edited_range("data")
        if gameplay is None and data is None:
            continue
        inputs = level / "bake-inputs"
        inputs.mkdir(exist_ok=True)

        if gameplay is not None:
            gameplay = decoded(gameplay)
            (inputs / "gameplay.bin").write_bytes(gameplay)
            mobys = [{"index": row["index"], "class": row["class_id"], "matrix": row["matrix"],
                      "fields": {"pvar_index": row["fields"].get("pvar_index")}}
                     for row in moby_instances(gameplay)]
            (inputs / "moby-placements.json").write_text(json.dumps({"mobys": mobys}), encoding="utf-8")

        if data is not None:
            index, core = (decoded(span(data, *struct.unpack_from("<ii", data, SECTION_NAMES.index(name) * 8)))
                           for name in ("core_index", "core_data"))
            offset, = struct.unpack_from("<I", index, 0x14)
            (inputs / "collision.glb").write_bytes(collision_glb(decode(core[offset:])))


def prepare_mod_inputs(image, root, levels):
    mounts = [root]
    destinations = root / "destinations"
    if destinations.is_dir():
        mounts.extend(path for path in sorted(destinations.iterdir()) if path.is_dir())
    mappings = [(mount, sector_files(mount, "patches.tsv"), sector_files(mount, "virtual.tsv"))
                for mount in mounts]
    # Texture-only stacks need no disc read or decode.
    edited = [(mount, fixed, virtual) for mount, fixed, virtual in mappings if fixed or virtual]
    if not edited:
        return
    manifest = min(levels.glob("level_*/manifest.json"), default=None)
    expected = json.loads(manifest.read_text(encoding="utf-8")).get("version") if manifest else None
    with Disc(image) as disc:
        if disc.version.key != expected:
            raise FormatError("the installed disc does not match the exported lighting levels")
        disc_levels = disc.survey()["levels"]
        for mount, fixed, virtual in edited:
            prepare_mount(disc, disc_levels, mount, fixed, virtual)
