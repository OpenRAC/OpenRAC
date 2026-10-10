"""The PAL v2.00 disc: its ISO 9660 files, sector table of contents and level headers.

Only three files are named on the disc. Everything else is addressed by
absolute 2048-byte sectors through the table the game reads at startup.
"""

import hashlib
from pathlib import Path

from formats import FormatError, span, unpack

SECTOR = 2048
EXE_NAME, EXE_SIZE = "SCES_509.16", 1388100
PAL_SHA1 = "79956931bd62fafd8d20fa2eae796dbaf2e15e83"
TOC_LBA, TOC_SIZE = 1500, 0x2960       # func_0012F3F8 into D_00137C80
LEVEL_TABLE = 0x28c8                    # D_0013A548, read by func_0012F4A8
LEVEL_COUNT = 19
LEVEL_HEADER_SIZE = 0x2434
LEVEL_RANGES = ("data", "gameplay_ntsc", "gameplay_pal", "occlusion")

# Global table groups: (offset, slots, name, unit of the second word).
# Names follow Wrench's RacWadInfo; "lba" groups store sectors only.
GROUPS = (
    (0x008, 1, "debug_font", "sectors"), (0x010, 1, "save_game", "sectors"),
    (0x018, 28, "ratchet_sequences", "sectors"), (0x0f8, 20, "hud_sequences", "sectors"),
    (0x198, 1, "vendor", "sectors"), (0x1a0, 37, "vendor_audio", "sectors"),
    (0x2c8, 12, "help_controls", "sectors"), (0x328, 15, "help_moves", "sectors"),
    (0x3a0, 15, "help_weapons", "sectors"), (0x418, 14, "help_gadgets", "sectors"),
    (0x488, 7, "help_ss", "sectors"), (0x4c0, 7, "options_ss", "sectors"),
    (0x4f8, 1, "frontbin", "sectors"), (0x500, 81, "mission_ss", "sectors"),
    (0x788, 19, "planets", "sectors"), (0x820, 38, "unknown_stuff2", "sectors"),
    (0x950, 10, "goodies_images", "sectors"), (0x9a0, 19, "character_sketches", "sectors"),
    (0xa38, 19, "character_renders", "sectors"), (0xad0, 31, "skill_images", "sectors"),
    (0xbc8, 60, "epilogue_images", "sectors"), (0xda8, 30, "sketchbook", "sectors"),
    (0xe98, 4, "commercials", "sectors"), (0xeb8, 9, "item_images", "sectors"),
    (0xf00, 240, "qwark_boss_audio", "lba"), (0x12c0, 1, "irx", "sectors"),
    (0x12c8, 4, "spaceships", "sectors"), (0x12e8, 20, "unknown_animation", "sectors"),
    (0x1388, 6, "space_plates", "sectors"), (0x13b8, 1, "transition", "sectors"),
    (0x13c0, 36, "space_audio", "sectors"), (0x14e0, 1, "sound_bank", "sectors"),
    (0x14e8, 1, "unknown_wad", "sectors"), (0x14f0, 1, "music", "sectors"),
    (0x14f8, 1, "hud_header", "sectors"), (0x1500, 5, "hud_banks", "sectors"),
    (0x1528, 1, "all_text", "sectors"), (0x1530, 28, "unknown_things", "sectors"),
    (0x1610, 1, "post_credits_sequence", "sectors"),
    (0x1618, 18, "post_credits_audio", "sectors"),
    (0x16a8, 20, "credits_images_ntsc", "sectors"),
    (0x1748, 20, "credits_images_pal", "sectors"),
    (0x17e8, 2, "unknown_wads", "sectors"), (0x17f8, 88, "mpeg", "bytes"),
    (0x1ab8, 900, "help_audio", "lba"),
)


class Disc:
    """Read-only access to an ISO image, bounded by its size."""

    def __init__(self, path: Path):
        self.file = path.open("rb")
        self.size = path.stat().st_size

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.file.close()

    def check(self, offset: int, size: int = 0) -> None:
        if offset < 0 or size < 0 or offset + size > self.size:
            raise FormatError(f"disc range {offset:#x}+{size:#x} outside the ISO")

    def read(self, offset: int, size: int) -> bytes:
        self.check(offset, size)
        if size > 64 << 20:
            raise FormatError("disc read larger than 64 MiB")
        self.file.seek(offset)
        data = self.file.read(size)
        if len(data) != size:
            raise FormatError("short ISO read")
        return data

    def sectors(self, lba: int, count: int) -> bytes:
        return self.read(lba * SECTOR, count * SECTOR)

    def files(self) -> list[dict]:
        """The root directory; this disc has no subdirectories."""
        pvd = self.read(16 * SECTOR, SECTOR)
        if pvd[:7] != b"\x01CD001\x01" or unpack("<H", pvd, 128)[0] != SECTOR:
            raise FormatError("expected ISO 9660 with 2048-byte sectors")
        lba, size = unpack("<I", pvd, 158)[0], unpack("<I", pvd, 166)[0]
        directory = self.read(lba * SECTOR, size)
        records, pos = [], 0
        while pos < len(directory):
            length = directory[pos]
            if not length:  # Records never cross a sector; zeros pad to the next.
                pos = (pos // SECTOR + 1) * SECTOR
                continue
            record = span(directory, pos, length)
            name = span(record, 33, span(record, 0, 33)[32])
            pos += length
            if name in (b"\0", b"\1"):
                continue
            if record[25] & 0x82:
                raise FormatError("unexpected subdirectory or multi-extent file")
            records.append({"name": name.decode("ascii").split(";")[0],
                            "lba": unpack("<I", record, 2)[0],
                            "bytes": unpack("<I", record, 10)[0]})
        return records

    def executable(self) -> bytes:
        """SCES_509.16, the game's executable."""
        exe = next((f for f in self.files() if f["name"] == EXE_NAME), None)
        if exe is None:
            raise FormatError(f"the disc has no {EXE_NAME}")
        return self.read(exe["lba"] * SECTOR, exe["bytes"])

    def survey(self) -> dict:
        """Check the disc is PAL v2.00 and list every sector reference.

        References are not unique files: entries may alias. Sizes the table
        does not store stay None rather than being inferred.
        """
        files = self.files()
        exe = next((f for f in files if f["name"] == EXE_NAME), None)
        if exe is None or exe["bytes"] != EXE_SIZE:
            raise FormatError("only SCES_509.16 (PAL v2.00) is supported")
        digest = hashlib.sha1(self.read(exe["lba"] * SECTOR, exe["bytes"])).hexdigest()
        if digest != PAL_SHA1:
            raise FormatError("the executable's SHA-1 does not match PAL v2.00")
        toc = self.read(TOC_LBA * SECTOR, TOC_SIZE)
        if unpack("<II", toc) != (1, TOC_SIZE):
            raise FormatError("unexpected table of contents header")
        refs = []
        for offset, slots, name, unit in GROUPS:
            for i in range(slots):
                at = offset + i * (4 if unit == "lba" else 8)
                lba, = unpack("<I", toc, at)
                if not lba:
                    continue
                size = None
                if unit != "lba":
                    size = unpack("<I", toc, at + 4)[0] * (SECTOR if unit == "sectors" else 1)
                self.check(lba * SECTOR, size or 0)
                refs.append({"group": name, "index": i, "toc_offset": at, "lba": lba, "bytes": size})
        levels = [self.level_header(toc, i) for i in range(LEVEL_COUNT)]
        return {"schema": 1, "disc_bytes": self.size, "executable_sha1": digest,
                "iso_files": files, "global_references": refs, "levels": levels}

    def level_header(self, toc: bytes, level: int) -> dict:
        """A level's header: its four sector ranges plus audio and scene references."""
        lba, toc_size = unpack("<II", toc, LEVEL_TABLE + level * 8)
        header = self.read(lba * SECTOR, LEVEL_HEADER_SIZE)
        if unpack("<II", header) != (level, LEVEL_HEADER_SIZE):
            raise FormatError(f"unexpected level {level} header")
        ranges = {}
        for i, name in enumerate(LEVEL_RANGES):
            start, count = unpack("<II", header, 8 + i * 8)
            self.check(start * SECTOR, count * SECTOR)
            ranges[name] = {"lba": start, "sectors": count}
        refs = []
        for i in range(36):  # Sector/byte-size pairs.
            start, size = unpack("<II", header, 0x28 + i * 8)
            if start:
                self.check(start * SECTOR, size)
                refs.append({"group": "bindata", "index": i, "lba": start, "bytes": size})
        for i in range(15 + 30 * 74):  # 15 music LBAs, then 30 scenes of 6 audio + 68 WAD LBAs.
            start, = unpack("<I", header, 0x148 + i * 4)
            if not start:
                continue
            self.check(start * SECTOR)
            if i < 15:
                refs.append({"group": "music", "index": i, "lba": start, "bytes": None})
            else:
                scene, item = divmod(i - 15, 74)
                refs.append({"group": "scene_audio" if item < 6 else "scene_wad",
                             "scene": scene, "index": item, "lba": start, "bytes": None})
        return {"id": level, "header_lba": lba, "toc_size_sectors": toc_size,
                "ranges": ranges, "references": refs}
