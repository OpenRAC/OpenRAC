"""Going Commando's liquid surfaces (the lava, the water) for the offline baker.

A surface is a moby of class 4959 (SURFACE_CLASS): its update (func_00302A88) registers its draw,
func_00303070, in the third draw callback list, which draws an 8 x 8 vertex grid (7 x 7 quads) of
the effect texture its pvars name (+0x48, plus 0x3F: func_0027C878's index), its vertices'
colours from the wave height (80 + 48 x height of 128, so 80 at rest) and its texture repeating
every 1 / (+0x28) cells. func_00303210 makes the grid: the four corner markers, mobys of class 0x1130
(CONTROL_CLASS) in the surface's moby group, each give one corner's x and y by the selector at their
pvar +0x14 (0 to 3), and the selector 0 marker its height. The grid is a bilinear quad: a point at
column c and row r of 7 is left + (right - left) x r/7, left = corner 0 + (corner 1 - corner 0) x
c/7, right = corner 3 + (corner 2 - corner 3) x c/7. Read in level 14 (Aranos prison) against a
running game's memory: the corners and heights match the six surfaces it drew.

Nothing is derived here but the layout; whether a surface emits light is the reviewed list in
port/enhanced/lighting/profiles/rac2-ntsc/liquids.json, by the exported effect texture's SHA-256.
"""

import hashlib
import json
import struct

from formats import FormatError, png, png_chunk
from mobys import moby_instances

SURFACE_CLASS = 4959
CONTROL_CLASS = 0x1130
PVAR_TABLE, PVAR_DATA = 0x5C, 0x60      # the gameplay header's pvar table and data (Going Commando)
EFFECT_TABLE, EFFECT_BANK = 0x58, 0x68  # the core index's effect textures and their bank
EFFECT_BASE = 0x3F                      # func_00303070 adds it to the pvar's texture number
REST_TINT = 80                          # a vertex's colour at rest, of 128


def pvar(gameplay: bytes, index: int) -> bytes:
    table, data = struct.unpack_from("<2i", gameplay, PVAR_TABLE)
    if index < 0 or table <= 0 or data <= 0:
        return b""
    offset, size = struct.unpack_from("<2i", gameplay, table + 8 * index)
    if offset < 0 or size <= 0 or data + offset + size > len(gameplay):
        raise FormatError(f"pvar {index} outside the gameplay file")
    return gameplay[data + offset:data + offset + size]


def effect_texture(index: bytes, core: bytes, number: int) -> tuple[int, int, bytes, bytes]:
    """Effect texture NUMBER: width, height, its 8-bit pixels and its 256 RGBA palette entries,
    in stored order (the effect renderer reads them as stored, as Ratchet & Clank's Gaspar lava)."""
    count, table = struct.unpack_from("<2I", index, EFFECT_TABLE)
    bank, = struct.unpack_from("<I", index, EFFECT_BANK)
    if not 0 <= number < count:
        raise FormatError(f"effect texture {number} past the table's {count}")
    palette, pixels, width, height = struct.unpack_from("<4i", index, table + 16 * number)
    if palette < 0 or pixels < 0 or not (0 < width <= 512 and 0 < height <= 512):
        raise FormatError(f"effect texture {number}: unsupported entry")
    p0, x0 = bank + palette, bank + pixels
    if x0 + width * height > len(core) or p0 + 1024 > len(core):
        raise FormatError(f"effect texture {number} outside the core data")
    return width, height, core[x0:x0 + width * height], core[p0:p0 + 1024]


def effect_png(width: int, height: int, pixels: bytes, palette: bytes) -> bytes:
    colours = [palette[i * 4:i * 4 + 4] for i in range(256)]
    return png(width, height, 3, pixels,
               png_chunk(b"PLTE", b"".join(c[:3] for c in colours)),
               png_chunk(b"tRNS", bytes(min(255, c[3] * 2) for c in colours)))


def surfaces(level, fmt) -> list[dict]:
    """The level's liquid surfaces: corners, height, texture number and repeat, in instance order."""
    if len(level.gameplay) < 0xA0:
        return []  # no header: a synthetic level
    mobys = moby_instances(level.gameplay, fmt)
    markers = {}
    for m in mobys:
        if m["class_id"] == CONTROL_CLASS:
            markers.setdefault(m["fields"].get("group", -1), []).append(m)
    out = []
    for m in mobys:
        if m["class_id"] != SURFACE_CLASS:
            continue
        group = m["fields"].get("group", -1)
        state = pvar(level.gameplay, m["fields"].get("pvar_index", -1))
        corners, height = [None] * 4, None
        for marker in markers.get(group, []) if group >= 0 else []:
            settings = pvar(level.gameplay, marker["fields"].get("pvar_index", -1))
            selector, = struct.unpack_from("<I", settings, 0x14) if len(settings) >= 0x18 else (0,)
            selector = selector if selector <= 3 else 0  # func_00303210 folds others to 0
            corners[selector] = [marker["matrix"][12], marker["matrix"][13]]
            if selector == 0:
                height = marker["matrix"][14]
        if len(state) < 0x50 or None in corners or height is None:
            continue  # no grid: the game zeroes it, and draws nothing a light could come from
        texture, flags = struct.unpack_from("<2I", state, 0x48)
        repeat, = struct.unpack_from("<f", state, 0x28)
        out.append({"instance": m["index"], "group": group, "corners": corners, "height": height,
                    "texture": texture + EFFECT_BASE, "flags": flags, "repeat": repeat})
    return out


def write_liquids(level, fmt, directory, reviewed) -> dict | None:
    """bake-inputs/liquids.json and the effect textures it names; None for a level without any.
    REVIEWED: the SHA-256s of the effect textures that give off light."""
    found = surfaces(level, fmt)
    if not found:
        return None
    index, core = level.index, level.core  # the core with its texture base, as level.textures reads it
    textures = {}
    for surface in found:
        name = f"effect_{surface['texture']:04}.png"
        if name not in textures:
            width, height, pixels, palette = effect_texture(index, core, surface["texture"])
            textures[name] = (effect_png(width, height, pixels, palette), pixels, palette)
        image, pixels, palette = textures[name]
        sha = hashlib.sha256(image).hexdigest()
        surface["image"] = name
        surface["emissive"] = sha in reviewed
        if surface["emissive"]:
            surface["emission"] = average_colour(pixels, palette, REST_TINT / 128)
    for name, (image, _, _) in textures.items():
        (directory / name).write_bytes(image)
    (directory / "liquids.json").write_text(json.dumps({"version": 1, "surfaces": found}, indent=1) + "\n")
    return {"liquids": "rac2-surfaces-v1"}


def linear(c: float) -> float:
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def display(c: float) -> float:
    return c * 12.92 if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055


def average_colour(pixels: bytes, palette: bytes, tint: float) -> list[float]:
    """The texture's linear average, modulated by the vertex tint, back in display units: what the
    baker's emission sampling takes for a vertex colour over a white texture (liquids.cpp)."""
    counts = [0] * 256
    for p in pixels:
        counts[p] += 1
    total = len(pixels)
    out = []
    for c in range(3):
        acc = sum(n * linear(min(1.0, palette[i * 4 + c] / 255 * tint)) for i, n in enumerate(counts) if n)
        out.append(round(display(acc / total), 6))
    return out


# Up Your Arsenal's liquid surfaces (its pools, rivers and lava). A surface is a moby of class 6576
# (RAC3_SURFACE_CLASS): its update registers its draw (level 14's 0x003B24C8, no pair in Going
# Commando) in the draw callback list run after the ties, which draws one triangle strip through the
# points of a path of the gameplay file (the pvar's +0x3C names it; the paths are the section header
# word 0x78 points at): a point's height plus half the wave amplitude (+0x90, an integer) at rest.
# The strip's first layer is effect texture 0x16, white, tinted by the pvar's first colour (bytes 0
# to 3, 0x80 = 1.0) towards its last (the alpha byte 3's 0x80 bit picks the TEST: 0x5310B, as Going
# Commando's Dobbo water, when it is set); so a surface looks the colour its pvar names, and that is
# what the reviewed list (port/enhanced/lighting/profiles/rac3-pal/liquids.json) names it by: its
# first colour word. Read in level 14 against a running game's memory: its pool's 23 points are path
# 121 of the file, and the window draws it before the mobys as five strips of 23 vertices.
RAC3_SURFACE_CLASS = 6576
RAC3_PATHS = 0x78       # the gameplay header word of the paths section
RAC3_PATH_INDEX = 0x3C  # the pvar's path number
RAC3_WAVE = 0x90        # the pvar's wave amplitude, an integer
RAC3_REST = 128         # a colour byte at full strength


def path_points(gameplay: bytes, index: int) -> list[list[float]]:
    """Path INDEX of the gameplay file: its points, x, y, z. The section opens with the path count,
    the offset of its data (from the section's start) and its size; the paths' offsets in the data
    follow from +0x10. A path is its point count, 12 bytes, then a 16-byte x, y, z, w per point."""
    at, = struct.unpack_from("<I", gameplay, RAC3_PATHS)
    if at <= 0 or at + 0x10 > len(gameplay):
        return []
    count, data = struct.unpack_from("<2I", gameplay, at)
    if not 0 <= index < count:
        raise FormatError(f"path {index} past the file's {count}")
    offset, = struct.unpack_from("<I", gameplay, at + 0x10 + 4 * index)
    start = at + data + offset
    points, = struct.unpack_from("<I", gameplay, start)
    if points > 4096 or start + 0x10 + 16 * points > len(gameplay):
        raise FormatError(f"path {index} outside the gameplay file")
    return [list(struct.unpack_from("<3f", gameplay, start + 0x10 + 16 * i)) for i in range(points)]


def rac3_surfaces(level, fmt) -> list[dict]:
    """The level's liquid surfaces: their strip's points at rest and their first colour, in
    instance order."""
    if len(level.gameplay) < 0xA0:
        return []  # no header: a synthetic level
    out = []
    for m in moby_instances(level.gameplay, fmt):
        if m["class_id"] != RAC3_SURFACE_CLASS:
            continue
        state = pvar(level.gameplay, m["fields"].get("pvar_index", -1))
        if len(state) < RAC3_WAVE + 4:
            continue
        path, = struct.unpack_from("<i", state, RAC3_PATH_INDEX)
        wave, = struct.unpack_from("<i", state, RAC3_WAVE)
        points = path_points(level.gameplay, path) if path >= 0 else []
        if len(points) < 3:
            continue  # no strip
        rise = int(wave / 2)  # the draw's integer halving, toward zero
        out.append({"instance": m["index"], "path": path, "colour": state[0:4].hex(),
                    "points": [[x, y, z + rise] for x, y, z in points]})
    return out


def write_rac3_liquids(level, fmt, directory, reviewed) -> dict | None:
    """bake-inputs/liquids.json for Up Your Arsenal; None for a level without any. REVIEWED: the
    first colour words (8 hex digits) of the surfaces that give off light."""
    found = rac3_surfaces(level, fmt)
    if not found:
        return None
    for surface in found:
        surface["emissive"] = surface["colour"] in reviewed
        if surface["emissive"]:
            rgb = bytes.fromhex(surface["colour"])[:3]
            # A white texture under the colour, as the draw's first layer is.
            surface["emission"] = [round(min(1.0, c / RAC3_REST), 6) for c in rgb]
    (directory / "liquids.json").write_text(json.dumps({"version": 1, "surfaces": found}, indent=1) + "\n")
    return {"liquids": "rac3-strips-v1"}
