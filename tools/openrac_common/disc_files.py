# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""
The disc as files, and the map the port reads them by.

The games read their data by absolute sector, most of it outside the file system: Ratchet & Clank
names three files and reads everything else through its table of contents at sector 1500; Up Your
Arsenal names its executables and network files and keeps its WADs in sectors no file covers. So
the extractor writes two kinds of file into iso_data/GAME/:

  - every file of the disc's file system, at its path (SYSTEM.CNF, G/LEVEL0.WAD, I5BOOTN.ELF);
  - every other sector, in lumps/: cut where the game's own table of contents says a lump starts
    (lumps/toc.bin, lumps/global/..., lumps/levels/NN/...), the sectors no table names in
    lumps/sectors/<first sector>.bin;

and the sector map, sectors.tsv, which says for each run of sectors the file it is in:

  # openrac sector map 1: first sector, sectors, bytes, file
  0	289	591872	lumps/sectors/0000000.bin
  289	1	58	SYSTEM.CNF

A row's file holds BYTES bytes; its sectors past them read as zeros (the padding after a file of
the file system, which the extractor checks is zeros: a sector whose padding is not is a lump of
its own). The rows cover every sector of the image, in order, without overlapping, so the port
(port/assets/disc/sector_map.h) gives the game the very bytes the disc has at any sector it
asks for, without opening the image.

Where a game's table of contents is known (editor/disc.py, editor/wads.py) its lumps are named;
when the editor's readers are not there or do not know the disc, the sectors are cut at the
file system's files only, which the port reads the same way.

Up Your Arsenal's two programs (its boot executable and I5BOOTN.ELF, the multiplayer game) are a
loader carrying the game WAD-compressed at file offset 0x7080: they are unpacked here, into
programs/<file>.records (the loader's records, as port/game/rac3-pal/host/exec.c copies them to
memory), so the port never unpacks a program from the disc at run time.
"""
import sys
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO

SECTOR = 2048
MAP_NAME = "sectors.tsv"
MAP_VERSION = 1
MAP_HEADER = f"# openrac sector map {MAP_VERSION}: first sector, sectors, bytes, file"
LUMPS = "lumps"
PROGRAMS = "programs"
COPY_BLOCK = 1 << 23

# Up Your Arsenal's loaders: the packed game at file offset 0x7080 (port/game/rac3-pal/host/exec.c).
RAC3_PAYLOAD_OFFSET = 0x7080
RAC3_PROGRAMS = ("I5BOOTN.ELF",)


@dataclass(frozen=True)
class Piece:
    """One row of the map: SECTORS sectors from FIRST, in PATH (relative), which holds BYTES."""
    first: int
    sectors: int
    bytes: int
    path: str


def _editor() -> Path | None:
    """The checkout's (or the standalone copy's) editor/, whose readers know the tables of contents."""
    here = Path(__file__).resolve().parents[2] / "editor"
    return here if (here / "disc.py").is_file() else None


def toc_lumps(image: Path, log: Callable[[str], None] = print) -> dict[int, str]:
    """first sector -> lump name (no extension) of every lump the disc's table of contents names,
    from the editor's readers of it; {} when they are not there or do not know this disc."""
    editor = _editor()
    if editor is None:
        return {}
    if str(editor) not in sys.path:
        sys.path.insert(1, str(editor))
    try:
        import disc as editor_disc  # noqa: PLC0415 (only when the editor is there)
        import wads  # noqa: PLC0415
        from formats import unpack  # noqa: PLC0415
    except ImportError as e:
        log(f"  lumps: the editor's readers cannot be loaded ({e}): cut at the file system's files only")
        return {}
    starts: dict[int, str] = {}
    try:
        with editor_disc.Disc(Path(image)) as d:
            version = wads.identify(d)
            if version is None:
                version = d.version
                starts[version.toc_lba] = "toc"
                survey = d.survey()
                for ref in survey["global_references"]:
                    starts.setdefault(ref["lba"], f"global/{ref['group']}/{ref['index']:03}")
                for level in survey["levels"]:
                    folder = f"levels/{level['id']:02}"
                    starts.setdefault(level["header_lba"], f"{folder}/header")
                    for name, r in level["ranges"].items():
                        if r["sectors"]:
                            starts.setdefault(r["lba"], f"{folder}/{name}")
                    for ref in level["references"]:
                        scene = f"{ref['scene']:02}_" if "scene" in ref else ""
                        starts.setdefault(ref["lba"], f"{folder}/{ref['group']}/{scene}{ref['index']:03}")
            else:
                w = wads.WadDisc(d, version)
                starts[wads.TOC_LBA] = "toc"
                end, at, n = w.level_table(), 0, 0
                while at + 8 <= end:
                    size, lba = unpack("<ii", w.toc, at)
                    if size == 0:
                        at += 4
                        continue
                    if lba > wads.TOC_LBA:
                        starts.setdefault(lba, f"global/{n:03}")
                    n += 1
                    at += size
                for level in w.levels():
                    for kind in (wads.LEVEL, wads.AUDIO, wads.SCENE):
                        if kind in level:
                            starts.setdefault(level[kind]["lba"], f"levels/{level['id']:02}/{kind}")
    except Exception as e:  # noqa: BLE001 (a disc the readers do not know is cut by files only)
        log(f"  lumps: the table of contents cannot be read ({e}): cut at the file system's files only")
        return {}
    return starts


def plan(files: list[tuple[str, int, int]], sectors: int, lumps: dict[int, str]) -> list[Piece]:
    """The map's rows for an image of SECTORS sectors whose file system has FILES ((path, first
    sector, size)) and whose table of contents names LUMPS: the files where they are (a file that
    overlaps one before it is written but not mapped), every other sector a lump. Padding is not
    looked at here: extract() splits a file whose last sector has more than zeros after it."""
    rows: list[Piece] = []
    used: set[str] = set()
    covered = 0

    def lump(first: int, count: int) -> None:
        name = lumps.get(first, f"sectors/{first:07}")
        path = f"{LUMPS}/{name}.bin"
        if path in used:
            path = f"{LUMPS}/{name}.{first}.bin"
        used.add(path)
        rows.append(Piece(first, count, count * SECTOR, path))

    def gap(first: int, end: int) -> None:
        cuts = sorted(s for s in lumps if first < s < end)
        for start, stop in zip([first, *cuts], [*cuts, end]):
            lump(start, stop - start)

    for path, lba, size in sorted(files, key=lambda f: (f[1], f[0])):
        count = (size + SECTOR - 1) // SECTOR
        if count == 0 or lba < covered or lba + count > sectors:
            continue
        if lba > covered:
            gap(covered, lba)
        rows.append(Piece(lba, count, size, path))
        used.add(path)
        covered = lba + count
    if covered < sectors:
        gap(covered, sectors)
    return rows


def copy_range(src: BinaryIO, offset: int, size: int, dest: Path) -> None:
    """SIZE bytes of SRC at OFFSET into DEST."""
    dest.parent.mkdir(parents=True, exist_ok=True)
    src.seek(offset)
    left = size
    with open(dest, "wb") as out:
        while left:
            block = src.read(min(left, COPY_BLOCK))
            if not block:
                raise ValueError(f"{dest.name} runs past the end of the image")
            out.write(block)
            left -= len(block)


def padding_is_zero(src: BinaryIO, piece: Piece) -> bool:
    """Whether the bytes after a file of the file system, to the end of its last sector, are zeros."""
    tail = piece.sectors * SECTOR - piece.bytes
    if tail == 0:
        return True
    src.seek(piece.first * SECTOR + piece.bytes)
    return src.read(tail) == bytes(tail)


def write_lumps(src: BinaryIO, rows: list[Piece], out: Path, is_file: Callable[[str], bool],
                log: Callable[[str], None] = print) -> list[Piece]:
    """Writes every lump of ROWS under OUT (the file system's files, IS_FILE, are already there)
    and returns the rows as mapped: a file whose padding is not zeros keeps its whole sectors in
    the map and its last sector becomes a lump of its own."""
    final: list[Piece] = []
    written = total = 0
    for piece in rows:
        if is_file(piece.path):
            if padding_is_zero(src, piece):
                final.append(piece)
                continue
            whole = piece.bytes // SECTOR
            if whole:
                final.append(Piece(piece.first, whole, whole * SECTOR, piece.path))
            last = piece.first + whole
            path = f"{LUMPS}/sectors/{last:07}.bin"
            copy_range(src, last * SECTOR, SECTOR, out / path)
            final.append(Piece(last, 1, SECTOR, path))
            log(f"  {piece.path}: its last sector's padding is not zeros, kept as {path}")
            continue
        copy_range(src, piece.first * SECTOR, piece.bytes, out / piece.path)
        final.append(piece)
        written += 1
        total += piece.bytes
    log(f"  {LUMPS}/: {written} lumps, {total >> 20} MB")
    return final


def write_map(rows: list[Piece], out: Path) -> Path:
    """OUT/sectors.tsv for ROWS."""
    path = out / MAP_NAME
    lines = [MAP_HEADER] + [f"{p.first}\t{p.sectors}\t{p.bytes}\t{p.path}" for p in rows]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return path


def read_map(folder: Path) -> list[Piece]:
    """The rows of FOLDER/sectors.tsv (ValueError when it is not a map of this version)."""
    text = (folder / MAP_NAME).read_text(encoding="utf-8").splitlines()
    if not text or text[0] != MAP_HEADER:
        raise ValueError(f"{folder / MAP_NAME} is not a sector map of version {MAP_VERSION}")
    rows = []
    for line in text[1:]:
        if line and not line.startswith("#"):
            first, count, size, path = line.split("\t", 3)
            rows.append(Piece(int(first), int(count), int(size), path))
    return rows


def unpack_programs(game: str, out: Path, log: Callable[[str], None] = print,
                    boot: str | None = None) -> list[str]:
    """Up Your Arsenal's programs unpacked into OUT/programs/<file>.records (the module comment):
    the boot executable BOOT and I5BOOTN.ELF, from their extracted files in OUT. Nothing for the
    other games. Raises ValueError when a loader holds no packed game."""
    if game != "rac3":
        return []
    editor = _editor()
    if editor is None:
        raise ValueError("Up Your Arsenal's programs need the editor's WAD reader (editor/formats.py)")
    if str(editor) not in sys.path:
        sys.path.insert(1, str(editor))
    from formats import wad  # noqa: PLC0415
    done = []
    for name in (*([boot] if boot else []), *RAC3_PROGRAMS):
        src = out / name
        if not src.is_file():
            continue
        data = src.read_bytes()
        if len(data) <= RAC3_PAYLOAD_OFFSET + 16 or data[RAC3_PAYLOAD_OFFSET:RAC3_PAYLOAD_OFFSET + 3] != b"WAD":
            raise ValueError(f"{name} carries no packed game at {RAC3_PAYLOAD_OFFSET:#x}")
        records = wad(data[RAC3_PAYLOAD_OFFSET:])
        dest = out / PROGRAMS / f"{name}.records"
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(records)
        log(f"  {PROGRAMS}/{name}.records: {len(records)} bytes, unpacked from {name}")
        done.append(name)
    return done


def write_disc(image: Path, out: Path, game: str, boot: str | None = None,
               log: Callable[[str], None] = print) -> list[Piece]:
    """Everything the port reads of IMAGE, into OUT, for a set-up that has only the image (the iOS
    importer's): the file system's files not already there, the lumps, GAME's programs and the map,
    written last. tools/extractor.py does the same with its own checks of each file."""
    from openrac_common import iso9660  # noqa: PLC0415 (iso9660 imports nothing of this)
    entries = iso9660.files(image)
    with open(image, "rb") as f:
        for path, lba, size in entries:
            if not (out / path).is_file():
                copy_range(f, lba * SECTOR, size, out / path)
        rows = plan(entries, Path(image).stat().st_size // SECTOR, toc_lumps(Path(image), log))
        rows = write_lumps(f, rows, out, {path for path, _, _ in entries}.__contains__, log)
    unpack_programs(game, out, log, boot=boot)
    write_map(rows, out)
    return rows
