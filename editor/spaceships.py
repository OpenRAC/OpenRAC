"""The player's ships, which every level can show: the disc's spaceships files (global table
0x12c8), entry k + 1 holding ship class 531 + k and its extra class 535 + k, each with one
texture. The level loader copies the file and adds both classes to the level's moby classes
(entry 0, class 530, only Veldin 1 has, in its own core). As ReRAC reads them
(crates/rc-formats/src/moby_spawn.rs, parse_spaceship; ISC License, Copyright (c) 2026 ReRAC
contributors): a word header (+0 the ship class, +4 its texture list, +8 the extra class, +0xc
its texture list), each texture list one PIF ("2FIP" at +0x10, its size at +0x18, format 0x13 at
+0x20) whose CLUT is at +0x30 (16 x 16 PSMCT32, CSM1) and pixels at +0x430.
"""

from disc import SECTOR, Disc
from formats import FormatError, Texture, unpack
from level import Level, decoded
from moby_class import MobyClass, moby_class

SHIP_CLASSES = (531, 532, 533)
EXTRA_CLASSES = (535, 536, 537)
# Texture keys of their own, past any level's moby textures.
TEXTURE_BASE = 1000


def spaceship_texture(blob: bytes, at: int, size: int) -> Texture:
    count, first = unpack("<II", blob, at)
    width, height, fmt = unpack("<III", blob, at + 0x18)
    if count != 1 or first != 0x10 or blob[at + 0x10:at + 0x14] != b"2FIP" or (width, height, fmt) != (size, size, 0x13):
        raise FormatError(f"spaceship texture at {at:#x}: not one {size}x{size} PSMT8 picture")
    return Texture(size, size, blob[at + 0x430:at + 0x430 + size * size], blob[at + 0x30:at + 0x430])


def add_spaceships(disc: Disc, survey: dict, level: Level) -> dict[int, MobyClass]:
    """The ship classes and their extra classes, decoded; their textures go into level.textures."""
    refs = {r["index"]: r for r in survey["global_references"] if r["group"] == "spaceships"}
    classes = {}
    for k, (ship_id, extra_id) in enumerate(zip(SHIP_CLASSES, EXTRA_CLASSES)):
        ref = refs.get(k + 1)
        if ref is None or not ref["bytes"]:
            continue
        blob = decoded(disc.sectors(ref["lba"], (ref["bytes"] + SECTOR - 1) // SECTOR))
        ship, ship_tex, extra, extra_tex = unpack("<IIII", blob)
        if not ship < extra < ship_tex < extra_tex < len(blob):
            raise FormatError(f"spaceships {k + 1}: blocks out of order")
        for class_id, start, end, tex_at, size, key in (
                (ship_id, ship, extra, ship_tex, 256, TEXTURE_BASE + 2 * k),
                (extra_id, extra, ship_tex, extra_tex, 128, TEXTURE_BASE + 2 * k + 1)):
            level.textures[("moby", key)] = spaceship_texture(blob, tex_at, size)
            classes[class_id] = moby_class(blob[start:end], [key] * 16, f"moby_{class_id}")
    return classes
