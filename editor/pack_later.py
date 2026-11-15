"""Pack Going Commando's levels edited in Godot back into the game's level data.

    python3 editor/extract.py pack <Going Commando disc> assets/levels/rac2/ntsc assets/packed/rac2/ntsc

The same output as Ratchet & Clank's packer (pack.py), for the port's `--mods`:

    <out>/level_NN/gameplay_<version>.wad   the level's gameplay file, compressed
    <out>/level_NN/header_<version>.bin     the level header's sector, when the file outgrew its place
    <out>/patches.tsv, virtual.tsv          the disc sectors each file replaces, and the sectors past the disc
    <out>/changes.txt                       what was changed, in words
    <out>/packed.json                       the game and disc it was packed from

A Going Commando level header sits in the table of contents (wads.py); its gameplay range is
relative to the level file's first sector. What it packs, for now: mobys moved, turned, scaled or
given other values (the rc1_* metadata). A moby added, deleted or given another class is listed
as not packed: those rewrite the tables that name mobys by their place (Ratchet & Clank's
gameplay.py), which Going Commando's gameplay file lays out otherwise.
"""

import json
import struct
from pathlib import Path

from disc import SECTOR
from formats import FormatError
import games
import gamedata
from level import decoded
from mobys import moby_instances
from pack import (GAME_FRAME, apply, same, scene_mobys, validate_packed_visibility, virtual_range)
import progress_line
from wadpack import compress

GAMEPLAY_FIELD = 0x20   # the level header's gameplay range: sector (from the file's first), count


def table(gameplay: bytes, fmt: games.Format) -> int:
    """The offset of the first moby record in the gameplay file."""
    start, = struct.unpack_from("<I", gameplay, fmt.mobys)
    return start + 0x10


def pack_level(gameplay: bytes, scene: str, fmt: games.Format) -> tuple[bytes, list[str], list[str]]:
    """The gameplay file with the scene's moby edits, what changed, and what could not be packed."""
    originals = moby_instances(gameplay, fmt)
    layout = fmt.moby
    record_format = struct.Struct(layout.layout)
    first = table(gameplay, fmt) if originals else 0
    edited = bytearray(gameplay)
    changes, problems, seen = [], [], set()
    for node in scene_mobys(scene):
        index = node["meta"].get("index")
        if not isinstance(index, int) or not 0 <= index < len(originals) or index in seen:
            problems.append(f"{node['name']}: a moby added in Godot; adding is Ratchet & Clank's only for now")
            continue
        seen.add(index)
        at = first + index * layout.size
        record = dict(zip(layout.fields, record_format.unpack_from(gameplay, at)))
        if node["class_id"] is not None and node["class_id"] != record["class_id"]:
            problems.append(f"{node['name']}: given class {node['class_id']}; changing a class is "
                            "Ratchet & Clank's only for now")
            node = {**node, "class_id": None}
        edits = apply(node, record, originals[index], True, problems, layout.usual)
        if edits:
            record_format.pack_into(edited, at, *(record[f] for f in layout.fields))
            changes.append(f"{node['name']}: {', '.join(edits)}")
    deleted = len(originals) - len(seen)
    if deleted:
        problems.append(f"{deleted} moby(s) deleted in Godot; deleting is Ratchet & Clank's only for now, "
                        "so they stay")
    return bytes(edited), changes, problems


def pack(source, survey: dict, project: Path, out: Path) -> tuple[int, int]:
    """Writes OUT from the scenes of PROJECT for SOURCE (wads.WadDisc). Returns (levels packed, mobys changed)."""
    fmt = games.FORMATS[survey["game"]]
    key = survey["version"]
    disc = source.disc
    patches, virtual, report, packed, total = [], [], [], 0, 0
    next_virtual = [0x10000000]

    def scene_of(entry: dict) -> Path | None:
        name = f"level_{entry['id']:02}"
        return next((s for s in (project / "levels" / name / f"{name}.tscn",
                                 project / name / "levels" / name / f"{name}.tscn") if s.is_file()), None)
    found = [(e, s) for e in survey["levels"] if "gameplay" in e.get("ranges", {}) and (s := scene_of(e))]
    for entry, scene in progress_line.counted("pack", found, lambda item: item[0]["id"]):
        name = f"level_{entry['id']:02}"
        part, place = entry["level"], entry["ranges"]["gameplay"]
        lba, sectors = part["lba"] + place["sector"], place["sectors"]
        stored = disc.sectors(lba, sectors)
        gameplay = decoded(stored)
        text = scene.read_text()
        nodes, _ = gamedata.parse_scene(text)
        frame = nodes.get("Game")
        if frame is not None and not same(gamedata.transform_of(frame), GAME_FRAME):
            raise FormatError(f"level {entry['id']:02}: keep the Game node's extracted transform; it converts "
                              "Godot's coordinates to the game's", code="pack-incomplete")
        validate_packed_visibility(nodes, entry["id"])
        edited, changes, problems = pack_level(gameplay, text, fmt)
        if changes or problems:
            report.append(f"level {entry['id']:02}")
            report += [f"  {line}" for line in changes]
            report += [f"  not packed: {line}" for line in problems]
        if not changes:
            continue
        stream = compress(edited, stored[:16])
        if decoded(stream) != edited:
            raise FormatError(f"level {entry['id']}: the packed file does not read back", code="pack-internal")
        file = f"{name}/gameplay_{key}.wad"
        (out / file).parent.mkdir(parents=True, exist_ok=True)
        if len(stream) <= sectors * SECTOR:
            (out / file).write_bytes(stream + bytes(sectors * SECTOR - len(stream)))
            patches.append(f"{lba}\t{sectors}\t{file}")
        else:
            # Past the disc's sectors, and the level header (its sector in the table of contents)
            # pointed there; the disc image is never changed.
            count = -(-len(stream) // SECTOR)
            first = virtual_range(next_virtual, count)
            (out / file).write_bytes(stream + bytes(count * SECTOR - len(stream)))
            virtual.append(f"{first}\t{count}\t{file}")
            header = bytearray(disc.sectors(part["header_lba"], 1))
            struct.pack_into("<ii", header, GAMEPLAY_FIELD, first - part["lba"], count)
            head = f"{name}/header_{key}.bin"
            (out / head).write_bytes(bytes(header))
            patches.append(f"{part['header_lba']}\t1\t{head}")
            report.append(f"  gameplay file grew to {len(stream)} bytes: read from past the disc")
        packed += 1
        total += len(changes)
    out.mkdir(parents=True, exist_ok=True)
    (out / "patches.tsv").write_text("# first sector\tsectors\tfile (beside this list)\n"
                                     + "".join(p + "\n" for p in patches))
    (out / "virtual.tsv").write_text("# first sector\tsectors\tfile (beside this list)\n"
                                     + "".join(p + "\n" for p in virtual))
    (out / "changes.txt").write_text("\n".join(report) + "\n" if report else "No level differs from the disc.\n")
    (out / "packed.json").write_text(json.dumps({"format": 1, "game": fmt.game, "version": key}) + "\n")
    return packed, total
