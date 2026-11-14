"""Imported moby classes: a moby of a class from another level, which runs as it does there.

A moby under Game/Mobys with rc1_foreign_level metadata (the level the class is from) and the class
of a moby scene the level does not have; rc1_foreign_class_scale is its class's scale, which the
editor draws it at (its own scale is the node's over that). The port registers the class when the level loads, from its
data as its own level's core has it, and runs its update in its own level's program, swapped in for
the call (port/game/rac1/host/foreign_class.c). Pack writes, beside the level's placements:

    <out>/level_NN/classes.tsv           one "class<TAB>level" a line
    <out>/level_NN/classes/<class>.bin   the class's data from its level's core

and places the moby with a copy of the first moby of that class in its own level: its fields and its
pvar block. The mobys its links name there come with it (linked), as does each class its update
can make that the level lacks (spawns.py), and its level's tables, moved to where it stands, go
to worlds.bin (world, write_worlds). Rac1 only, on any of its discs. Any level's class can come
into any other: only its level's data is swapped in for the call, and every level's data ends
below every level's program end.
"""

from dataclasses import dataclass, field
import os
import struct
from pathlib import Path

import gameplay as gp
from mobys import FIELDS, RECORD, moby_instances
from pack import scene_mobys

LEVELS = 19
DISCS = ("ntsc", "pal", "pal1", "jp")   # the port runs them on each (rac1_version.h, RAC1_FOREIGN_CLASSES)


@dataclass
class Donor:
    """A moby in its own level: what a copy of it here takes."""
    level: int
    class_id: int
    record: dict
    pvar: bytes | None = None
    pointers: list[int] = field(default_factory=list)   # pointer fixups in the block (offsets)
    links: list[int] = field(default_factory=list)      # moby links in the block (offsets)
    shared: list[tuple[int, bytes]] = field(default_factory=list)   # (offset in the block, its shared data)
    index: int = -1                                      # its place in its level's table
    linked: list[tuple[int, int]] = field(default_factory=list)     # (offset, the moby it names there)
    grouped: list[tuple[int, str, list[int]]] = field(default_factory=list)  # (offset, format, its group's mobys)


# Classes that run only inside their level's own set piece: what drives them there does not come
# along, and without it the game stops (found by placing every class in another level).
SET_PIECES = {
    (1843, 17): "this ship gets its path from the fleet turret (class 347); place the turret, it brings its ships",
    (1319, 11): "this ship gets its path from the oil tanker (class 1264); place the tanker, it brings its ships",
}

# The engine's 224 moby class slots (resident_class_id_by_slot) less two: after the level's classes
# and the imports, the texture bank registers the hero's two armor classes, with no check.
CLASS_SLOTS = 224 - 2


def wanted(text: str) -> tuple[dict[int, int], list[str]]:
    """The scene's imported classes (class -> its level), and what is wrong with the others."""
    out, problems = {}, []
    for node in scene_mobys(text):
        level = node["meta"].get("foreign_level")
        if level is None:
            continue
        if node["class_id"] is None:
            problems.append(f"{node['name']}: has rc1_foreign_level but no class scene")
            continue
        try:
            level = int(str(level).strip().strip('"'))
        except ValueError:
            problems.append(f"{node['name']}: rc1_foreign_level must be a level number, not {level!r}")
            continue
        if out.setdefault(node["class_id"], level) != level:
            problems.append(f"{node['name']}: class {node['class_id']} already comes from level "
                            f"{out[node['class_id']]:02}; a class can come from one level only")
    return out, problems


_class_tables: dict = {}


def level_list(levels: list[int]) -> str:
    """LEVELS short, with runs joined: [0, 2, 4, 5, 6] -> "00, 02, 04-06"."""
    out, i = [], 0
    while i < len(levels):
        j = i
        while j + 1 < len(levels) and levels[j + 1] == levels[j] + 1:
            j += 1
        out.append(f"{levels[i]:02}" if j - i < 2 else f"{levels[i]:02}-{levels[j]:02}")
        if j - i == 1:
            out.append(f"{levels[j]:02}")
        i = j + 1
    return ", ".join(out)


def levels_with_room(disc, survey: dict, classes: set[int], here: int) -> list[int]:
    """The levels (not HERE) whose free class slots hold all of CLASSES they lack."""
    from level import load_level
    out = []
    for info in survey["levels"]:
        if info["id"] == here:
            continue
        key = (id(disc), info["id"])
        if key not in _class_tables:
            try:
                _class_tables[key] = {struct.unpack_from("<i", e, 4)[0] for e in load_level(disc, info).table(0x18, 32)}
            except Exception:   # a level that cannot be read is just not named
                _class_tables[key] = None
        own = _class_tables[key]
        if own is not None and len(classes - own) <= CLASS_SLOTS - len(own):
            out.append(info["id"])
    return out


def partial(text: str) -> set[int]:
    """The imported classes the scene lets come without what does not fit in the level's class
    slots (rc1_foreign_partial on one of their mobys, the Inspector's switch)."""
    return {node["class_id"] for node in scene_mobys(text)
            if node["meta"].get("foreign_level") is not None
            and str(node["meta"].get("foreign_partial", "")).strip().lower() in ("true", "1")}


def donor(level: int, gameplay: bytes, class_id: int) -> Donor | None:
    """The first moby of CLASS_ID in LEVEL's gameplay file, or None."""
    first = next((m for m in moby_instances(gameplay) if m["class_id"] == class_id), None)
    return None if first is None else donor_at(level, gameplay, first["index"])


def donor_at(level: int, gameplay: bytes, index: int) -> Donor:
    """The moby at INDEX in LEVEL's gameplay file."""
    record = dict(zip(FIELDS, RECORD.unpack(gp.records(gameplay)[index])))
    out = Donor(level, record["class_id"], record, index=index)
    source = record["pvar_index"]
    links, pointers = gp.fixups(gameplay, gp.LINKS), gp.fixups(gameplay, gp.POINTERS)
    used = [m["fields"]["pvar_index"] for m in moby_instances(gameplay)] + [p for p, _ in links + pointers]
    pvars = gp.pvar_table(gameplay, gp.pvar_count(gameplay, used))
    if 0 <= source < len(pvars):
        data_start, _ = gp.extent(gameplay, gp.DATA)
        at, size = pvars[source]
        block = bytearray(gameplay[data_start + at:data_start + at + size])
        out.links = [where for pvar, where in links if pvar == source]
        out.pointers = [where for pvar, where in pointers if pvar == source]
        for where in out.links:
            if where + 4 <= len(block):
                named, = struct.unpack_from("<i", block, where)
                if named >= 0:
                    out.linked.append((where, named))
                struct.pack_into("<i", block, where, -1)
        # The words that hold a moby group's number (groups.py, from the game's code): the group's
        # mobys come with it, and pack.py gives the copy a group of its own of their copies.
        import groups as group_words
        words = group_words.fields(record["class_id"], level)
        groups = gp.groups(gameplay) if words else []
        for where, size in words:
            form = {1: "<b", 2: "<h", 4: "<i"}[size]
            if where + struct.calcsize(form) <= len(block):
                number, = struct.unpack_from(form, block, where)
                if 0 <= number < len(groups) and groups[number]:
                    out.grouped.append((where, form, list(groups[number])))
                struct.pack_into(form, block, where, -1)
        out.pvar = bytes(block)
        # Its shared data (a part of the file's blob that its block points at): each piece runs to
        # the next piece any block points at, or the blob's end.
        blob, records = gp.shared(gameplay)
        starts = sorted({into for _, _, into in records} | {len(blob)})
        for pvar, where, into in records:
            if pvar == source and 0 <= into < len(blob):
                end = next(s for s in starts if s > into)
                out.shared.append((where, bytes(blob[into:end])))
    return out


LINKED_MAX = 24   # the mobys a moby brings along with it, at most


def linked(level: int, gameplay: bytes, first: Donor) -> list[Donor]:
    """FIRST and the mobys its moby links and groups name in its level, and theirs, ..., FIRST first
    (at most LINKED_MAX more): what it needs to run as it does there (Qwark's lift, a turret's hatch)."""
    out, seen, count = [first], {first.index: first}, len(gp.records(gameplay))
    i = 0
    while i < len(out):
        for named in [n for _, n in out[i].linked] + [n for _, _, ns in out[i].grouped for n in ns]:
            if named not in seen and 0 <= named < count and len(out) <= LINKED_MAX:
                seen[named] = donor_at(level, gameplay, named)
                out.append(seen[named])
        i += 1
    return out


def class_blob(level, class_id: int) -> bytes:
    """CLASS_ID's data in LEVEL's core (a level.Level), as the level's loader registers it: empty
    for a class with none (a controller or trigger with no model), which the game registers so."""
    for entry in level.table(0x18, 32):
        offset, number = struct.unpack_from("<Ii", entry, 0)
        if number == class_id:
            return level.block(offset) if offset else b""
    raise ValueError(f"level {level.id:02} has no class {class_id}")


def check(here: int, there: int, class_id: int) -> None:
    """ValueError when CLASS_ID cannot come from level THERE into level HERE."""
    if (class_id, there) in SET_PIECES and not os.environ.get("OPENRAC_PACK_SET_PIECES"):   # (testing them)
        raise ValueError(f"class {class_id}: {SET_PIECES[(class_id, there)]}")
    if not 0 <= there < LEVELS:
        raise ValueError(f"class {class_id}: there is no level {there}")
    if there == here:
        raise ValueError(f"class {class_id}: it is already from this level")


def write(out: Path, blobs: dict[int, tuple[int, bytes]]) -> list[str]:
    """Writes classes.tsv and the class blobs into OUT (the packed level's folder): what was done."""
    folder = out / "classes"
    if folder.is_dir():
        for old in folder.glob("*.bin"):
            old.unlink()
    if not blobs:
        if (out / "classes.tsv").exists():
            (out / "classes.tsv").unlink()
        return []
    folder.mkdir(parents=True, exist_ok=True)
    # In BLOBS' order: the port registers them so, and when the level's class slots run out the
    # last ones (spawned shots and parts) are the ones left out, not the placed class.
    lines = ["# class\tlevel (editor/foreign_classes.py)"]
    for class_id, (level, blob) in blobs.items():
        (folder / f"{class_id}.bin").write_bytes(blob)
        lines.append(f"{class_id}\t{level}")
    (out / "classes.tsv").write_text("\n".join(lines) + "\n")
    return [f"class {c}: imported from level {lvl:02}" for c, (lvl, _) in blobs.items()]


# Its own level's tables (worlds.bin): what the port puts in place of this level's for its update
# (port/game/rac1/host/foreign_class.c, k_world_globals: these are their numbers there).
CAMERAS, CAMERA_COUNT, RAILS, RAIL_COUNT = 0, 1, 2, 3
CLIPS, CLIP_COUNT, CUBOIDS, CUBOID_COUNT = 4, 5, 6, 7
SPHERES, SPHERE_COUNT, CYLINDERS, CYLINDER_COUNT = 8, 9, 10, 11
PATH_COUNT, PATH_DATA, PATHS = 12, 13, 14
# The volumes: their global, header word, size in memory, size in the file (a clip volume is read
# 0x90 bytes at a time, 0x80 apart: its radius at +0x80 is the next one's first word).
SHAPES = ((CLIPS, 0x6C, 0x90, 0x80), (CUBOIDS, 0x60, 0x80, 0x80), (SPHERES, 0x64, 0x80, 0x80),
          (CYLINDERS, 0x68, 0x80, 0x80))


@dataclass
class Move:
    """Where a moby of its own level now stands: turned by `turn` radians about z, then moved."""
    turn: float
    dx: float
    dy: float
    dz: float

    @staticmethod
    def between(there: dict, here: dict) -> "Move":
        """From the moby THERE (a record of its own level) to HERE (its record in this level)."""
        import math
        turn = here["rz"] - there["rz"]
        c, s = math.cos(turn), math.sin(turn)
        x, y = there["x"] * c - there["y"] * s, there["x"] * s + there["y"] * c
        return Move(turn, here["x"] - x, here["y"] - y, here["z"] - there["z"])

    def vector(self, x: float, y: float, z: float) -> tuple[float, float, float]:
        import math
        c, s = math.cos(self.turn), math.sin(self.turn)
        return x * c - y * s, x * s + y * c, z

    def point(self, x: float, y: float, z: float) -> tuple[float, float, float]:
        x, y, z = self.vector(x, y, z)
        return x + self.dx, y + self.dy, z + self.dz


def moved_pvar(donor: "Donor", move: "Move | None") -> bytes:
    """DONOR's pvar block with the words that hold a place in the world (positions.py) moved by
    MOVE, as its paths are: a lift's floors stay where they were around it."""
    import positions
    block = bytearray(donor.pvar)
    words = dict(positions.fields(donor.class_id, donor.level)) if move is not None else {}
    done = set()
    for at, axis in sorted(words.items()):
        if at in done or at + 4 > len(block):
            continue
        if axis == "x" and words.get(at + 4) == "y" and at + 8 <= len(block):
            # A point: turned with the moby, its z too when it follows.
            has_z = words.get(at + 8) == "z" and at + 12 <= len(block)
            x, y = struct.unpack_from("<2f", block, at)
            z = struct.unpack_from("<f", block, at + 8)[0] if has_z else 0.0
            nx, ny, nz = move.point(x, y, z)
            struct.pack_into("<2f", block, at, nx, ny)
            if has_z:
                struct.pack_into("<f", block, at + 8, nz)
            done |= {at, at + 4} | ({at + 8} if has_z else set())
            continue
        value, = struct.unpack_from("<f", block, at)
        struct.pack_into("<f", block, at, value + {"x": move.dx, "y": move.dy, "z": move.dz}[axis])
        done.add(at)
    return bytes(block)


def moved_shape(entry: bytes, move: Move) -> bytes:
    """A volume (its axes at +0, its place at +0x30, the axes' inverse at +0x40, its angles at
    +0x70) where MOVE puts it: the axes turned, the inverse turned with them."""
    import math
    out = bytearray(entry)
    f = list(struct.unpack_from("<32f", entry, 0))
    for row in range(3):
        f[row * 4:row * 4 + 3] = move.vector(*f[row * 4:row * 4 + 3])
    f[12:15] = move.point(*f[12:15])
    c, s = math.cos(move.turn), math.sin(move.turn)
    b0, b1 = f[16:19], f[20:23]
    f[16:19] = [c * u - s * v for u, v in zip(b0, b1)]
    f[20:23] = [s * u + c * v for u, v in zip(b0, b1)]
    f[30] += move.turn
    struct.pack_into("<32f", out, 0, *f)
    return bytes(out)


def moved_paths(blob: bytes, offsets: list[int], move: Move) -> bytes:
    """Path data (each path: its point count, then 16-byte points from +0x10) with MOVE's points."""
    out = bytearray(blob)
    for at in sorted(set(offsets)):
        if not 0 <= at <= len(out) - 16:
            continue
        count, = struct.unpack_from("<i", out, at)
        for i in range(max(0, min(count, (len(out) - at - 16) // 16))):
            where = at + 16 + i * 16
            struct.pack_into("<3f", out, where, *move.point(*struct.unpack_from("<3f", out, where)))
    return bytes(out)


def world(gameplay: bytes, move: Move) -> tuple[list[int], list[tuple[int, int, int, int]], bytes]:
    """LEVEL's tables (its gameplay file) as its loader lays them out, moved by MOVE: the offsets
    of the pointers in them, the globals to set ({global, index, value, value is an offset}), the
    data."""
    data, relocs, sets = bytearray(), [], []
    head = struct.unpack_from(f"<{gp.SECTIONS}i", gameplay, 0)

    def word(pointer: int) -> int:
        return head[pointer // 4]

    def put(piece: bytes) -> int:
        data.extend(bytes(-len(data) % 16))
        at = len(data)
        data.extend(piece)
        return at

    def table(global_: int, at: int | None, count: int) -> None:
        sets.append((global_, 0, 0 if at is None else at, 0 if at is None else 1))
        sets.append((global_ + 1, 0, count, 0))

    for global_, pointer, size, step in SHAPES:
        q = word(pointer)
        count = struct.unpack_from("<i", gameplay, q)[0] if q > 0 else 0
        pieces = [moved_shape(gameplay[q + 0x10 + i * step:q + 0x10 + i * step + size].ljust(size, b"\0"), move)
                  for i in range(count)]
        table(global_, put(b"".join(pieces)) if pieces else None, count)
    # Cameras: {pos, class, angles, its pvar block}.
    q = word(0x8)
    count = struct.unpack_from("<i", gameplay, q)[0] if q > 0 else 0
    if count:
        pvar_start, _ = gp.extent(gameplay, gp.DATA)
        table_at = word(gp.TABLE)
        cameras, blocks = bytearray(), []
        for i in range(count):
            kind, x, y, z, rx, ry, rz, pvar = struct.unpack_from("<i3f3fi", gameplay, q + 0x10 + i * 0x20)
            x, y, z = move.point(x, y, z)
            cameras += struct.pack("<3fi3fi", x, y, z, kind, rx, ry, rz + move.turn, 0)
            block = None
            if pvar >= 0 and table_at > 0 and pvar_start > 0:
                at, size = struct.unpack_from("<ii", gameplay, table_at + pvar * 8)
                block = bytes(gameplay[pvar_start + at:pvar_start + at + size])
            blocks.append(block)
        base = put(bytes(cameras))
        for i, block in enumerate(blocks):
            if block is not None:
                struct.pack_into("<i", data, base + i * 0x20 + 0x1C, put(block))
                relocs.append(base + i * 0x20 + 0x1C)
        table(CAMERAS, base, count)
    else:
        table(CAMERAS, None, 0)
    # Paths: their data, then each path's address in toad_paths.
    q = word(0x70)
    if q > 0:
        count, source, size = struct.unpack_from("<3i", gameplay, q)
        offsets = list(struct.unpack_from(f"<{count}i", gameplay, q + 0x10))
        base = put(moved_paths(gameplay[q + source:q + source + size], offsets, move))
        sets.append((PATH_COUNT, 0, count, 0))
        sets.append((PATH_DATA, 0, base, 1))
        sets += [(PATHS, i, base + off, 1) for i, off in enumerate(offsets)]
    else:
        sets += [(PATH_COUNT, 0, 0, 0), (PATH_DATA, 0, 0, 0)]
    # Rails: {its sphere, its path, its kind}, their paths' data after them.
    q = word(0x74)
    if q > 0:
        count, source, size = struct.unpack_from("<3i", gameplay, q)
        offsets = list(struct.unpack_from(f"<{count}i", gameplay, q + 0x10 + count * 0x20)) if count > 0 else []
        paths = put(moved_paths(gameplay[q + source:q + source + size], offsets, move))
        rails = bytearray()
        for i in range(max(count, 0)):
            x, y, z, radius = struct.unpack_from("<4f", gameplay, q + 0x10 + i * 0x20)
            kind, = struct.unpack_from("<i", gameplay, q + 0x20 + i * 0x20)
            rails += struct.pack("<4fIi8x", *move.point(x, y, z), radius, paths + offsets[i], kind)
        base = put(bytes(rails))
        relocs += [base + i * 0x20 + 0x10 for i in range(max(count, 0))]
        table(RAILS, base, max(count, 0))
    else:
        table(RAILS, None, 0)
    return relocs, sets, bytes(data)


def write_worlds(out: Path, placed: list[tuple[list[tuple[dict, Donor]], bytes]]) -> list[str]:
    """worlds.bin in OUT (the packed level's folder): for each imported moby (its record here and
    its Donor, then the same for each moby it brought along; its level's gameplay file), its
    level's tables moved to where it stands, and the mobys that use them.

        "ORWD", 1, worlds, bindings
        worlds   {at, size, relocs, sets}: at `at`, its relocs (u32 offsets in its data of words
                 its address is added to), its sets {global, index, value, value is an offset},
                 its data (`size` bytes)
        bindings {class, world, x, y}: the moby of that class packed at that x and y"""
    path = out / "worlds.bin"
    if not placed:
        if path.exists():
            path.unlink()
        return []
    heads, bindings, sections = [], [], []
    at = 16 + 16 * len(placed) + 16 * sum(len(copies) for copies, _ in placed)
    total = 0
    for w, (copies, gameplay) in enumerate(placed):
        record, model = copies[0]
        relocs, sets, data = world(gameplay, Move.between(model.record, record))
        section = (struct.pack(f"<{len(relocs)}I", *relocs)
                   + b"".join(struct.pack("<IIII", g, i, v & 0xFFFFFFFF, p) for g, i, v, p in sets) + data)
        heads.append(struct.pack("<4I", at, len(data), len(relocs), len(sets)))
        bindings += [struct.pack("<iI2f", r["class_id"], w, r["x"], r["y"]) for r, _ in copies]
        sections.append(section)
        at += len(section)
        total += len(data)
    out.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"ORWD" + struct.pack("<III", 1, len(placed), len(bindings))
                     + b"".join(heads) + b"".join(bindings) + b"".join(sections))
    return [f"{len(placed)} imported mobys bring {total >> 10} KB of their levels' data"]
