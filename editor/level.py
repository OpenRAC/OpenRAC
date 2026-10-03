"""Load one level from the disc: its data sections, core assets and textures."""

from bisect import bisect_right
from dataclasses import dataclass, field

from disc import LEVEL_HEADER_SIZE, SECTOR, Disc
from formats import FormatError, Texture, overlay_sections, span, unpack, wad

# The level data header: eleven (offset, size) pairs, names from Wrench's
# RacLevelDataHeader. The first feeds ParseBin through D_0015EF4C.
SECTION_NAMES = ("overlay", "sound_bank", "core_index", "gs_ram", "hud_header",
                 "hud_bank_0", "hud_bank_1", "hud_bank_2", "hud_bank_3",
                 "hud_bank_4", "core_data")
# Core index texture tables: (count, offset) pairs of 16-byte entries.
TEXTURE_TABLES = {"terrain": 0x30, "moby": 0x38, "tie": 0x40, "shrub": 0x48}
# Class tables: (count, offset) pairs; each entry starts with a core offset.
CLASS_TABLES = {"moby": (0x18, 32), "tie": (0x20, 32), "shrub": (0x28, 48)}


def decoded(raw: bytes) -> bytes:
    return wad(raw) if raw[:3] == b"WAD" else raw


@dataclass
class Level:
    id: int
    header: bytes
    stored: dict[str, bytes]      # Sections and ranges as they are on the disc.
    index: bytes                  # Core index, decoded.
    core: bytes                   # Core data, decoded.
    gameplay: bytes               # PAL gameplay, decoded.
    overlay: dict
    textures: dict[tuple[str, int], Texture] = field(default_factory=dict)
    boundaries: list[int] = field(default_factory=list)

    def table(self, at: int, size: int) -> list[bytes]:
        count, offset = unpack("<II", self.index, at)
        data = span(self.index, offset, count * size)
        return [data[i * size:(i + 1) * size] for i in range(count)]

    def block(self, start: int) -> bytes:
        """A core block, ending at the next known block (Wrench's bounds)."""
        i = bisect_right(self.boundaries, start)
        if i == len(self.boundaries):
            raise FormatError(f"cannot bound core block at {start:#x}")
        return span(self.core, start, self.boundaries[i] - start)

    def texture(self, group: str, index: int) -> Texture:
        if (group, index) not in self.textures:
            raise FormatError(f"missing {group} texture {index}")
        return self.textures[(group, index)]


def load_level(disc: Disc, info: dict) -> Level:
    ranges = info["ranges"]
    data = disc.sectors(ranges["data"]["lba"], ranges["data"]["sectors"])
    stored, sections = {}, {}
    for i, name in enumerate(SECTION_NAMES):
        offset, size = unpack("<ii", data, i * 8)
        if size:
            stored[name] = span(data, offset, size)
            sections[name] = decoded(stored[name])
    for name in ("gameplay_ntsc", "gameplay_pal", "occlusion"):
        stored[name] = disc.sectors(ranges[name]["lba"], ranges[name]["sectors"])
    for name in ("overlay", "core_index", "core_data", "gs_ram"):
        if name not in sections:
            raise FormatError(f"level {info['id']} has no {name} section")
    index, core = sections["core_index"], sections["core_data"]
    if len(core) != unpack("<I", index, 0x8c)[0]:
        raise FormatError("core data size differs from its index")
    level = Level(info["id"], disc.read(info["header_lba"] * SECTOR, LEVEL_HEADER_SIZE), stored,
                  index, core, decoded(stored["gameplay_pal"]), overlay_sections(sections["overlay"]))
    level.boundaries = core_boundaries(level)
    level.textures = textures(level, sections["gs_ram"])
    return level


def core_boundaries(level: Level) -> list[int]:
    words = [unpack("<I", level.index, at)[0] for at in (0x08, 0x0c, 0x10, 0x14, 0x60)]
    bounds = {len(level.core), *words}
    for at, size in CLASS_TABLES.values():
        bounds.update(unpack("<I", entry)[0] for entry in level.table(at, size))
    sequences, = unpack("<I", level.index, 0x78)
    if sequences:  # 256 ratchet sequence offsets, stored in the index.
        bounds.update(o for o in unpack("<256i", level.index, sequences) if o > 0)
    bounds.update(unpack("<I", entry)[0] for entry in level.table(0x80, 16))  # Gadgets.
    return sorted(b for b in bounds if 0 < b <= len(level.core))


def textures(level: Level, gs: bytes) -> dict[tuple[str, int], Texture]:
    """Base mip levels of every texture, checked against the GS RAM table.

    An entry is (pixel offset from the texture base, width, height, type,
    palette in 256-byte units, mips, pad). Palettes must be RGBA32 CLUTs
    (PSM 0) that the level's GS RAM table uploads.
    """
    base, = unpack("<I", level.index, 0x60)
    clut32 = {unpack("<I", e, 8)[0] for e in level.table(0x00, 16) if unpack("<I", e)[0] == 0}
    result = {}
    for group, at in TEXTURE_TABLES.items():
        for i, entry in enumerate(level.table(at, 16)):
            pixels, width, height, _kind, palette, _mips, _pad = unpack("<I6H", entry)
            if not width and not height:
                continue
            if palette * 256 not in clut32:
                raise FormatError(f"{group} texture {i} uses an unknown palette")
            result[(group, i)] = Texture(width, height, span(level.core, base + pixels, width * height),
                                         span(gs, palette * 256, 1024))
    return result
