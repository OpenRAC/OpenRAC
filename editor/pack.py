"""Pack the levels edited in Godot back into the game's level data.

    python3 editor/extract.py pack baserom/SCES_509.16.iso assets/levels/rac1/pal assets/packed/rac1/pal
    python3 editor/extract.py pack baserom/SCUS_971.99.iso assets/levels/rac1/ntsc assets/packed/rac1/ntsc

For each level whose scene differs from the disc, writes that level's
gameplay file (the placements) as the game stores it, and a list of the disc
sectors each file replaces. Either disc: each level header holds a gameplay
copy per video mode. Both native ports select the 60 Hz copy on their own disc
(disc.Version.gameplay), so that copy is the one read and replaced:

    <out>/level_NN/gameplay_<version>.wad   the level's gameplay file, compressed
    <out>/patches.tsv                 first sector, sector count, file: one line per file
    <out>/changes.txt                 what was changed, in words
    <out>/packed.json                 the disc it was packed from (overrides.py stacks it for that disc only)
    <out>/level_NN/cutscenes/         the level's cutscenes, as OpenRAC Editor wrote them beside its
                                      scene, for the editor-runtime mod (pack_cutscenes)

The disc image is never written. The native port reads `patches.tsv` (its
`--mods` option) and serves those sectors from the files instead of the disc.

What it packs, for now: mobys that were moved, turned, scaled, given another
class or other values (the `rc1_*` metadata), added (a copy made in Godot, or
a class scene of the level dropped in) or deleted; the level's collision
(collision_scene.py) and its moby classes' collision, edited in the class
scenes (moby_collision_scene.py: a class's edit is every moby of that class
in that level), both into the level data's core. Ties and shrubs are packed
for the native port, as the files it draws them from (scenery.py:
<out>/level_NN/placements.json). The gameplay data (pvars, links, groups, volumes,
paths, cameras, sounds, level settings) is gamedata.py's, called from pack_level. Terrain
is a later step, as are classes the level does not have.

A moby the scene has as the extractor wrote it keeps its bytes on the disc,
so a scene nobody edited packs to nothing, and a level whose mobys were only
edited differs from the disc's in those records alone. Adding and deleting
write the table again, and with it what names mobys by their place in it
(gameplay.py).
"""

import json
import math
import re
import struct
from pathlib import Path

from disc import LEVEL_HEADER_SIZE, LEVEL_RANGES, SECTOR
from formats import FormatError, unpack
from level import decoded, load_level
import gamedata
import progress_line
import gameplay as gp
from mobys import (ALWAYS, FIELDS, POINTER, RECORD, SIZE, USUAL, moby_class_names,
                   moby_instances, node_name)
from wadpack import compress

# Godot saves a node's header with more in it than the extractor writes (type, unique_id).
NODE = re.compile(r'^\[node name="([^"]+)"[^\]]* parent="Game/(\w+)"')
INSTANCE = re.compile(r'instance=ExtResource\("([^"]+)"\)')
RESOURCE = re.compile(r'^\[ext_resource [^\]]*path="([^"]+)"[^\]]*id="([^"]+)"')
CLASS_SCENE = re.compile(r"/(moby|tie|shrub)_(\d+)\.(?:tscn|glb)$")
PROPERTY = re.compile(r"^([A-Za-z_/0-9]+) = (.*)$")
RAC1_SPAWN_ID_COUNT = 0x800  # RAC1's 64-word per-level killed-state bitset.
VANILLA_RAM_BYTES = 32 * 1024 * 1024
EXPANDED_RAM_BYTES = 256 * 1024 * 1024  # The largest guest arena selected for virtual level streams.
IDENTITY = (1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0)
# The Godot `Game` node turns the editor's Y-up coordinates into RAC1's Z-up game space. Packers
# convert child transforms with scene_transform and therefore require this extracted frame intact.
GAME_FRAME = (1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0)
GROUPS_MAX = 112   # the moby groups the game loads (D_L00_001AB840, 0x1C0 bytes)
PACKED_VISIBILITY_GROUPS = (
    "Game/Ties",
    "Game/Shrubs",
    "Game/Mobys",
    "Game/StaticAssets",
    "Game/AnimatedAssets",
)


def f32(x: float) -> bytes:
    """The 32-bit float a number in the scene stands for; the scene writes -0 as 0."""
    return struct.pack("<f", x + 0.0 if x else 0.0)


def scene_mobys(text: str) -> list[dict]:
    return read_scene_nodes(text, "Mobys", "moby")


def read_scene_nodes(text: str, parent: str, kind: str) -> list[dict]:
    """The nodes of a level scene under Game/PARENT (Mobys, Ties, Shrubs): name, class id (of a
    KIND class scene: moby, tie, shrub), the 12 floats of its transform (basis rows, then
    origin) and its rc1_* metadata."""
    classes, nodes, node = {}, [], None
    for line in text.splitlines():
        if line.startswith("["):
            node = None
            resource = RESOURCE.match(line)
            if resource:
                found = CLASS_SCENE.search(resource.group(1))
                if found and found.group(1) == kind:
                    classes[resource.group(2)] = int(found.group(2))
                continue
            match = NODE.match(line)
            if match and match.group(2) == parent:
                instance = INSTANCE.search(line)
                node = {"name": match.group(1), "class_id": classes.get(instance.group(1)) if instance else None,
                        "transform": IDENTITY, "meta": {}}
                nodes.append(node)
            continue
        if node is None:
            continue
        prop = PROPERTY.match(line)
        if not prop:
            continue
        key, value = prop.groups()
        if key == "transform":
            inner = re.fullmatch(r"Transform3D\((.*)\)", value.strip())
            if not inner:
                raise FormatError(f"{node['name']}: cannot read its transform")
            floats = tuple(float(v) for v in inner.group(1).split(","))
            if len(floats) != 12:
                raise FormatError(f"{node['name']}: a transform has 12 numbers")
            node["transform"] = floats
        elif key.startswith("metadata/rc1_"):
            name, value = key[len("metadata/rc1_"):], value.strip()
            if value.startswith("["):
                try:
                    node["meta"][name] = [int(float(v)) for v in value.strip("[]").split(",") if v.strip()]
                except ValueError:      # a list of names or a multi-line one (gamedata.py's)
                    node["meta"][name] = value
            else:
                try:
                    number = float(value)
                    node["meta"][name] = int(number) if number == int(number) and "." not in value else number
                except (ValueError, OverflowError):     # text (a link's target) or not finite: gamedata.py's
                    node["meta"][name] = value
    return nodes


def scene_transform(m: list[float]) -> tuple:
    """A column-major matrix as the scene lists it: basis rows, then the origin."""
    return tuple([m[col * 4 + row] for row in range(3) for col in range(3)] + list(m[12:15]))


def near(x: float, y: float) -> bool:
    """The same 32-bit float, or its neighbour: the extractor writes the shortest text of a
    64-bit value, which for a value midway between two 32-bit floats can read back as the other."""
    a, b = (struct.unpack("<i", f32(v))[0] for v in (x, y))
    return abs(a - b) <= 1


def same(a: tuple, b: tuple) -> bool:
    """Equal as the 32-bit floats the scene's text stands for."""
    return all(near(x, y) for x, y in zip(a, b))


def same_value(a, b) -> bool:
    """Equal as the scene's text stands for them: a float is a 32-bit one."""
    if isinstance(a, float) or isinstance(b, float):
        return near(a, b)
    return a == b


def same_fields(a: dict, b: dict) -> bool:
    return a.keys() == b.keys() and all(same_value(a[k], b[k]) for k in a)


def validate_packed_visibility(nodes: dict, level_id: int) -> None:
    """Godot visibility is only an editor preview property; the game pack has no such flag."""
    for path, node in nodes.items():
        if not any(path.startswith(group + "/") or path == group for group in PACKED_VISIBILITY_GROUPS):
            continue
        value = node["props"].get("visible")
        if value is not None and gamedata.parse_value(value) is False:
            raise FormatError(
                f"level {level_id:02}: {path} is hidden in Godot, but visibility changes are not "
                "packed. Delete the object to remove it from the game.",
                code="pack-incomplete"
            )


def placement(t: tuple) -> tuple[tuple, tuple, float]:
    """Position, Euler angles (radians, R = Rz * Ry * Rx) and scale of a scene transform."""
    rows = [t[0:3], t[3:6], t[6:9]]
    scales = [math.sqrt(sum(rows[r][c] ** 2 for r in range(3))) for c in range(3)]
    scale = scales[0]
    if scale <= 0 or any(abs(s - scale) > 1e-3 * max(scale, 1.0) for s in scales):
        raise FormatError("a moby has one scale for all three axes")
    r = [[rows[i][j] / scale for j in range(3)] for i in range(3)]
    sy = max(-1.0, min(1.0, -r[2][0]))
    ry = math.asin(sy)
    if abs(sy) < 0.999999:
        rx, rz = math.atan2(r[2][1], r[2][2]), math.atan2(r[1][0], r[0][0])
    else:                                   # looking straight up or down: x and z turn about one axis
        rx, rz = math.atan2(-r[1][2], r[1][1]), 0.0
    return tuple(t[9:12]), (rx, ry, rz), scale


def set_expanded_range(disc, info: dict, headers: dict[int, bytearray], slot: int,
                       first: int, count: int) -> None:
    """Set one range in a level header, preserving other expanded ranges."""
    level = info["id"]
    if level not in headers:
        headers[level] = bytearray(disc.read(info["header_lba"] * SECTOR, LEVEL_HEADER_SIZE))
    struct.pack_into("<II", headers[level], 8 + slot * 8, first, count)


def virtual_range(next_sector: list[int], count: int) -> int:
    """Reserve non-overlapping sectors in the game-visible virtual LBA space."""
    first = next_sector[0]
    end = first + count
    if count <= 0 or end > 0xffffffff:
        raise FormatError("expanded level data exceeds the game's 32-bit sector address space")
    next_sector[0] = end
    return first


def apply(node: dict, record: dict, original: dict | None, whole: bool, problems: list[str]) -> list[str]:
    """Writes a scene node's placement, class and values into RECORD (a moby's fields). ORIGINAL is
    the moby the node was extracted from, when RECORD is still that moby's: what the scene has
    as the extractor wrote it is then left alone. WHOLE says the node lists every value that is
    not the usual one (a node the extractor wrote, or Godot's copy of one), so a value it does
    not list is the usual one. Returns what changed, in words."""
    edits = []
    if original is None or not same(node["transform"], scene_transform(original["matrix"])):
        (x, y, z), (rx, ry, rz), scale = placement(node["transform"])
        record.update(x=x, y=y, z=z, rx=rx, ry=ry, rz=rz, scale=scale)
        edits.append("moved")
    if node["class_id"] is not None and node["class_id"] != record["class_id"]:
        edits.append(f"class {record['class_id']} -> {node['class_id']}")
        record["class_id"] = node["class_id"]
    # Godot may save inherited class metadata on every instance. These facts describe
    # the class; they are not editable gameplay fields and must not become false edits.
    descriptive = ("index", "class", "class_name", "source", "update", "foreign_level", "foreign_class_scale")
    # Skins and behaviours are packed beside the placements (skins.py, behaviours.py).
    beside = ("skin", "behaviour")
    meta = {k: v for k, v in node["meta"].items()
            if k not in descriptive and not k.startswith(beside) and not gamedata.owns(k)}
    if original is None or not same_fields(meta, original["fields"]):
        for key in ALWAYS:
            if key in meta:
                record[key] = meta[key]
        if "colour" in meta:
            record["r"], record["g"], record["b"] = meta["colour"]
        for key, usual in USUAL.items():
            if whole or key in meta:
                record[key] = meta.get(key, usual)
        unknown = sorted(set(meta) - set(ALWAYS) - set(USUAL) - {"colour"})
        if unknown:
            problems.append(f"{node['name']}: no such value: rc1_{', rc1_'.join(unknown)}")
        edits.append("values")
    return edits


def pack_level(level_id: int, gameplay: bytes, scene: str, available_classes: set[int] | None = None,
               node_names: list[str] | None = None,
               spawn_ids: dict[str, int] | None = None,
               foreign: dict | None = None,
               foreign_placed: list | None = None) -> tuple[bytes, list[str], list[str]]:
    """The gameplay file with the scene's edits, what changed, and what could not be packed.

    A moby the scene has once keeps its place in the table. A node with the index of another
    (Godot's copy of a moby) or with none (a class scene dropped into the level) is a new moby,
    after the table's own: it takes after the moby it copies, or the level's first moby of its
    class, with a spawn number of its own and a copy of that moby's pvar block (and of the
    fixups, links and shared records that name it), and joins its group. A moby with no node
    left is deleted: the table closes up, and the groups and the pvars' moby links, which name
    mobys by their place in the table, follow. A moby of a class FOREIGN has (class -> its
    foreign_classes.Donor, a moby of it in its own level) takes after that moby, in no group.
    NODE_NAMES, when given, gets each packed moby's node name, in the table's order; SPAWN_IDS each packed moby's spawn id, by its node's name.
    FOREIGN_PLACED, when given, gets each moby of a class FOREIGN has: (its record, its Donor)."""
    originals = moby_instances(gameplay)
    raw = gp.records(gameplay)
    nodes = scene_mobys(scene)
    changes, problems = [], []

    def index_of(node: dict) -> int | None:
        index = node["meta"].get("index")
        return index if isinstance(index, int) and 0 <= index < len(originals) else None

    # The node that is each moby: the one still named after its place when Godot copied it.
    owner: dict[int, dict] = {}
    for node in nodes:
        index = index_of(node)
        if index is None:
            continue
        named = node["name"].endswith(f"_{index:04}")
        if index not in owner or (named and not owner[index]["name"].endswith(f"_{index:04}")):
            owner[index] = node

    kept: dict[int, bytes] = {}
    added: list[tuple[dict, int | None]] = []          # the new moby's fields, the moby it takes after
    added_names: list[str] = []                         # their nodes' names
    brought: list[tuple[dict, object]] = []             # mobys an imported moby brings along
    moved: dict[int, object] = {}                       # id of an imported copy's record -> its Move
    instances: list[list[tuple[dict, object]]] = []     # each imported moby and what it brings
    reclassed: dict[int, dict] = {}                     # kept mobys whose class changed: their fields
    for node in nodes:
        index = index_of(node)
        try:
            if index is not None and owner[index] is node:
                record = dict(zip(FIELDS, RECORD.unpack(raw[index])))
                target_class = node["class_id"]
                if target_class is not None and target_class != record["class_id"]:
                    # Its pvars are its old class's: it takes a copy of a moby of the new class's
                    # initialized block (below), keeping its place in the table.
                    donor = next((m for m in originals
                                  if m["class_id"] == target_class and m["index"] != index), None)
                    if donor is None and record["pvar_index"] >= 0:
                        problems.append(
                            f"{node['name']}: the level has no moby of class {target_class} to take "
                            "initialized pvars from, and its own are its old class's; its class is not changed. "
                            "Use a class already placed in this level, or import the model as static scenery."
                        )
                        kept[index] = raw[index]
                        continue
                    if donor is None:
                        problems.append(f"{node['name']}: the level has no moby of class {target_class} to "
                                        "take after; its class changed with no pvars")
                    edits = apply(node, record, originals[index], True, problems)
                    record["pvar_index"] = donor["fields"]["pvar_index"] if donor is not None else -1
                    reclassed[index] = record
                    kept[index] = RECORD.pack(*(record[f] for f in FIELDS))
                    changes.append(f"{node['name']}: {', '.join(edits)}")
                    continue
                edits = apply(node, record, originals[index], True, problems)
                kept[index] = RECORD.pack(*(record[f] for f in FIELDS)) if edits else raw[index]
                if edits:
                    changes.append(f"{node['name']}: {', '.join(edits)}")
                continue
            model = index
            if model is None and node["class_id"] is not None:
                model = next((m["index"] for m in originals if m["class_id"] == node["class_id"]), None)
            if model is None and node["class_id"] is None:
                problems.append(f"{node['name']}: added in Godot with no class scene; not packed")
                continue
            if model is None and foreign and node["class_id"] in foreign:
                group = foreign[node["class_id"]]
                donor = group[0]
                record = dict(donor.record, group=-1)
                apply(node, record, None, False, problems)
                # The editor draws it at its class's size (rc1_foreign_class_scale): the game
                # scales the class by the moby's own scale.
                class_scale = node["meta"].get("foreign_class_scale", 1.0)
                if isinstance(class_scale, (int, float)) and class_scale > 0:
                    record["scale"] /= class_scale
                added.append((record, donor))
                added_names.append(node["name"])
                # The mobys its links name in its level come with it, moved as it was.
                copies = [(record, donor)]
                import foreign_classes
                move = foreign_classes.Move.between(donor.record, record)
                moved[id(record)] = move
                for member in group[1:]:
                    extra = dict(member.record, group=-1)
                    extra["x"], extra["y"], extra["z"] = move.point(member.record["x"], member.record["y"],
                                                                    member.record["z"])
                    extra["rz"] = math.remainder(member.record["rz"] + move.turn, math.tau)
                    brought.append((extra, member))
                    moved[id(extra)] = move
                    copies.append((extra, member))
                instances.append(copies)
                if foreign_placed is not None:
                    foreign_placed.append(copies)
                changes.append(f"{node['name']}: added, class {node['class_id']} from level {donor.level:02}"
                               + (f", with {len(group) - 1} moby(s) it names there" if len(group) > 1 else ""))
                continue
            if model is None:
                record = dict.fromkeys(FIELDS, 0)
                record.update(USUAL, size=SIZE, class_id=node["class_id"], scale=1.0, group=-1, pvar_index=-1,
                              r=128, g=128, b=128)
                if available_classes is not None and node["class_id"] not in available_classes:
                    problems.append(
                        f"{node['name']}: class {node['class_id']} is not in this level's core class table; "
                        "native classes from another level are not supported yet. Use a class available in "
                        "this level or import the model as static scenery."
                    )
                elif available_classes is not None:
                    problems.append(
                        f"{node['name']}: class {node['class_id']} exists in this level's core, but there is "
                        "no placed instance to copy initialized pvars from; this class cannot be added safely. "
                        "Use a class with an existing instance or import the model as static scenery."
                    )
                else:
                    problems.append(f"{node['name']}: the level has no moby of class {node['class_id']} to take "
                                    "after; added with the usual values and no pvars")
            else:
                record = dict(zip(FIELDS, RECORD.unpack(raw[model])))
            source_pvar = record["pvar_index"] if model is not None else -1
            # A copy lists its values as the moby it copies does; a class scene dropped in lists none.
            apply(node, record, None, index is not None, problems)
            record["pvar_index"] = source_pvar          # the copy gets a block of its own below
            added.append((record, model))
            added_names.append(node["name"])
            changes.append(f"{node['name']}: added" + (f", after moby {model}" if model is not None else ""))
        except FormatError as e:
            problems.append(f"{node['name']}: {e}; not packed")
            if index is not None and owner[index] is node:
                kept[index] = raw[index]
    added += brought                                    # after the nodes' own (added_names)
    deleted = sorted(set(range(len(originals))) - set(kept))
    names = moby_class_names()
    for index in deleted:
        changes.append(f"{node_name(originals[index]['class_id'], index, names)}: deleted")

    if not added and not deleted and not reclassed:
        if kept and len(originals) > 0x8000:
            raise FormatError(
                f"level {level_id:02}: the edited gameplay table has {len(originals)} mobys; "
                "RAC1's signed 16-bit runtime indices support at most 32768. Remove mobys or "
                "split the map across levels before packing."
            )
        out = bytearray(gameplay)
        start, = unpack("<I", gameplay, POINTER)
        for index, record in kept.items():
            out[start + 0x10 + index * SIZE:start + 0x10 + (index + 1) * SIZE] = record
        # The gameplay data (pvars, links, volumes, paths, cameras...): gamedata.py.
        numbering = {owner[i]["name"]: i for i in kept}
        if node_names is not None:
            node_names[:] = sorted(numbering, key=numbering.get)
        if spawn_ids is not None:
            spawn_ids.update({owner[i]["name"]: dict(zip(FIELDS, RECORD.unpack(kept[i])))["spawn_id"] for i in kept})
        return gamedata.pack_level(gameplay, bytes(out), scene, numbering, changes, problems)

    # The table again: the mobys that stay, in their order, then the new ones.
    if len(kept) + len(added) > 0x8000:
        raise FormatError(
            f"level {level_id:02}: the packed gameplay table would have {len(kept) + len(added)} "
            "mobys; RAC1's signed 16-bit runtime indices support at most 32768. Remove mobys or "
            "split the map across levels before packing."
        )
    place = {old: new for new, old in enumerate(sorted(kept))}
    table = [kept[old] for old in sorted(kept)]
    group_lists = gp.groups(gameplay)
    group_lists = [None if g is None else [place[m] for m in g if m in place] for g in group_lists]
    links, pointers = gp.fixups(gameplay, gp.LINKS), gp.fixups(gameplay, gp.POINTERS)
    blob, shared_records = gp.shared(gameplay)
    used = [m["fields"]["pvar_index"] for m in originals] + [p for p, _ in links + pointers]
    pvars = gp.pvar_table(gameplay, gp.pvar_count(gameplay, used))
    data_start, data_end = gp.extent(gameplay, gp.DATA)
    data = bytearray(gameplay[data_start:data_end]) if data_start > 0 else bytearray()
    used_spawn_ids = {m["fields"]["spawn_id"] for m in originals
                      if 0 <= m["fields"]["spawn_id"] < RAC1_SPAWN_ID_COUNT}
    free_spawn_ids = [i for i in range(RAC1_SPAWN_ID_COUNT) if i not in used_spawn_ids]
    # A moby whose class changed: its own copy of the new class's pvar block, its links with it.
    for old, record in reclassed.items():
        source = record["pvar_index"]
        if 0 <= source < len(pvars):
            at, size = pvars[source]
            block = bytes(data[at:at + size])
            data += bytes(-len(data) % 16)
            new = len(pvars)
            pvars.append((len(data), size))
            data += block
            links += [(new, where) for pvar, where in links if pvar == source]
            pointers += [(new, where) for pvar, where in pointers if pvar == source]
            shared_records += [(new, where, into) for pvar, where, into in shared_records if pvar == source]
            record["pvar_index"] = new
        else:
            record["pvar_index"] = -1
        table[place[old]] = RECORD.pack(*(record[f] for f in FIELDS))
    without_save_id = 0
    placed_at: dict[int, int] = {}
    for record, model in added:
        if free_spawn_ids:
            record["spawn_id"] = free_spawn_ids.pop(0)
        else:
            # The game's per-level and session killed-state bitsets hold 2,048 IDs. Keep
            # excess additions usable as non-persistent mobys instead of indexing past them.
            record["spawn_id"] = -1
            record["spawn_flags"] = 0
            without_save_id += 1
        source = record["pvar_index"]
        if not isinstance(model, (int, type(None))):
            # Another level's moby: its own pvar block, its moby links naming none (foreign_classes.py).
            import foreign_classes
            record["pvar_index"] = -1
            if model.pvar is not None:
                data += bytes(-len(data) % 16)
                new = len(pvars)
                pvars.append((len(data), len(model.pvar)))
                data += foreign_classes.moved_pvar(model, moved.get(id(record)))
                links += [(new, where) for where in model.links]
                pointers += [(new, where) for where in model.pointers]
                for where, piece in model.shared:
                    blob += bytes(-len(blob) % 16)
                    shared_records.append((new, where, len(blob)))
                    blob += piece
                record["pvar_index"] = new
        elif 0 <= source < len(pvars):
            at, size = pvars[source]
            block = bytes(data[at:at + size])
            data += bytes(-len(data) % 16)
            new = len(pvars)
            pvars.append((len(data), size))
            data += block
            links += [(new, where) for pvar, where in links if pvar == source]
            pointers += [(new, where) for pvar, where in pointers if pvar == source]
            shared_records += [(new, where, into) for pvar, where, into in shared_records if pvar == source]
            record["pvar_index"] = new
        else:
            record["pvar_index"] = -1
        group = record["group"]
        if 0 <= group < len(group_lists):
            group_lists[group] = (group_lists[group] or []) + [len(table)]
        placed_at[id(record)] = len(table)
        table.append(RECORD.pack(*(record[f] for f in FIELDS)))
    if without_save_id:
        changes.append(f"{without_save_id} added mobys have no free RAC1 spawn ID; made non-persistent "
                       "(spawn ID -1, spawn flags 0)")
    # A link is a moby's place in the table, inside a pvar block: it follows the moby.
    for pvar, where in links:
        at = pvars[pvar][0] + where
        if at + 4 > len(data):
            continue
        moby, = struct.unpack_from("<i", data, at)
        if 0 <= moby < len(originals):
            struct.pack_into("<i", data, at, place.get(moby, -1))
    # Every level has the 112 groups the game loads (load_level_gameplay clears 0x1C0 bytes of
    # pointers), many empty: a new group takes an empty one past the last the level uses, from the
    # top, and a group with none of its mobys here is one empty group they all share.
    free: list[int] = []
    empty_group: list[int] = []
    started: list[bool] = []

    def new_group(members: list[int]) -> int:
        if not started:
            started.append(True)
            last_used = max((i for i, g in enumerate(group_lists) if g), default=-1)
            group_lists.extend([None] * (GROUPS_MAX - len(group_lists)))
            free.extend(i for i in range(last_used + 1, GROUPS_MAX) if not group_lists[i])
        if not members:
            if not empty_group and free:
                empty_group.append(free.pop(0))
            if empty_group:
                return empty_group[0]
        elif free:
            number = free.pop()
            group_lists[number] = members
            return number
        problems.append(f"no moby group left (the game has {GROUPS_MAX}): an imported moby's group is left out")
        return -1

    # An imported moby's links name the mobys it brought along (foreign_classes.linked).
    for copies in instances:
        by_index = {member.index: record for record, member in copies}
        for record, member in copies:
            if record["pvar_index"] < 0:
                continue
            base = pvars[record["pvar_index"]][0]
            for where, named in member.linked:
                target = by_index.get(named)
                if target is not None and id(target) in placed_at and base + where + 4 <= len(data):
                    struct.pack_into("<i", data, base + where, placed_at[id(target)])
            # Its groups (groups.py): a new group of the copies of their mobys, or an empty one.
            for where, form, members in member.grouped:
                inside = [placed_at[id(by_index[m])] for m in members
                          if m in by_index and id(by_index[m]) in placed_at]
                if base + where + struct.calcsize(form) <= len(data):
                    struct.pack_into(form, data, base + where, new_group(inside))
        # The group a copy is in, in its own level: one of the copies that were in it with it.
        own_groups: dict[int, list[dict]] = {}
        for record, member in copies:
            if 0 <= member.record["group"] < 0xFF and id(record) in placed_at:
                own_groups.setdefault(member.record["group"], []).append(record)
        for together in own_groups.values():
            number = new_group([placed_at[id(r)] for r in together])
            for record in together:
                record["group"] = number if number >= 0 else -1
                table[placed_at[id(record)]] = RECORD.pack(*(record[f] for f in FIELDS))

    imported = any(not isinstance(model, (int, type(None))) for _, model in added)
    sections = {gp.MOBYS: gp.write_records(gameplay, table)}
    for pointer, new in ((gp.GROUPS, gp.write_groups(group_lists) if group_lists else b""),
                         (gp.TABLE, b"".join(struct.pack("<2i", *entry) for entry in pvars)),
                         (gp.DATA, bytes(data)),
                         (gp.LINKS, gp.write_fixups(links)),
                         (gp.POINTERS, gp.write_fixups(pointers)),
                         (gp.SHARED, gp.write_shared(blob, shared_records))):
        start = gp.offset(gameplay, pointer)
        if start > 0 and new and gameplay[start:start + len(new)] != new:
            sections[pointer] = new
        elif start <= 0 and imported and new and new not in (gp.write_fixups([]), gp.write_shared(b"", [])):
            sections[pointer] = new     # another level's moby brings a section this level lacks
    numbering = {owner[old]["name"]: new for old, new in place.items()}
    numbering.update({name: len(kept) + k for k, name in enumerate(added_names)})
    if node_names is not None:
        node_names[:] = sorted(numbering, key=numbering.get)
    if spawn_ids is not None:
        spawn_ids.update({owner[old]["name"]: dict(zip(FIELDS, RECORD.unpack(kept[old])))["spawn_id"]
                          for old in kept})
        spawn_ids.update({name: record["spawn_id"] for name, (record, _) in zip(added_names, added)})
    return gamedata.pack_level(gameplay, gp.replace(gameplay, sections), scene, numbering, changes, problems)


def pack_collision(disc, info: dict, scene: Path, text: str, out: Path, patches: list[str],
                   virtual: list[str], headers: dict[int, bytearray], next_virtual: list[int]
                   ) -> tuple[list[str], list[str], bool]:
    """The level's collision and its moby classes' collision as the scene and the class scenes
    have them: when they differ from the disc's, the level data's sectors that change (the core
    data stream, and the core index when a block moved or the core grew) go to
    OUT/level_NN/{core,index}_<version>.bin with their lines in PATCHES. Returns (changes, problems,
    whether files were written)."""
    import collision_scene
    import moby_collision_scene
    level_dir, project = scene.parent, scene.parents[2]
    changes, problems = [], []
    try:
        found = collision_scene.scene_collision(project, level_dir, text)
    except (FormatError, ValueError) as e:
        found = None
        problems.append(f"collision: {e}")
    level = None
    new = None
    force_expanded_ram = False
    if found is not None:
        level = load_level(disc, info)
        try:
            new, changes, more = collision_scene.pack_block(level.block(unpack("<I", level.index, 0x14)[0]), found)
            force_expanded_ram = new is not None and collision_scene.requires_expanded_collision(new)
            problems += more
        except FormatError as e:
            problems += found["problems"] + [f"{e}; the collision is left as the disc has it"]
    classes = {}
    if not moby_collision_scene.unchanged_scenes(level_dir):
        level = level or load_level(disc, info)
        classes, class_changes, class_problems = moby_collision_scene.pack_classes(level, project, level_dir)
        changes += class_changes
        problems += class_problems
    if new is None and not classes:
        return changes, problems, False
    core, index, notes = moby_collision_scene.core_with_classes(level.core, level.index, level, classes)
    if new is not None:
        at, = unpack("<I", level.index, 0x14)
        core, index, more = collision_scene.core_with(core, index, at, len(level.block(at)), new)
        notes += more
    if len(core) > EXPANDED_RAM_BYTES:
        raise FormatError(f"level {info['id']:02}: decoded core data is {len(core)} bytes; it cannot fit in "
                          "the 256 MiB expanded guest RAM arena", code="level-too-big")
    data = disc.sectors(info["ranges"]["data"]["lba"], info["ranges"]["data"]["sectors"])
    expanded = collision_scene.expanded_level_data(data, core, index, level.index, compress,
                                                   force=force_expanded_ram or len(core) > VANILLA_RAM_BYTES)
    core_stream = compress(core, data[struct.unpack_from("<i", data, 10 * 8)[0]:][:16])
    changes.append(
        f"level core size: {len(core)} bytes decoded; {len(core_stream)} compressed; "
        f"original core stream slot {struct.unpack_from('<i', data, 10 * 8 + 4)[0]} bytes"
    )
    name = f"level_{info['id']:02}"
    if expanded is not None:
        changes.append(f"expanded level-data container: {len(expanded)} bytes before sector padding")
        count = -(-len(expanded) // SECTOR)
        first = virtual_range(next_virtual, count)
        file = f"{name}/data_{disc.version.key}.bin"
        (out / file).parent.mkdir(parents=True, exist_ok=True)
        (out / file).write_bytes(expanded + bytes(count * SECTOR - len(expanded)))
        virtual.append(f"{first}\t{count}\t{file}\t{info['id']}\t0\t{info['ranges']['data']['lba']}\t"
                       f"{info['ranges']['data']['sectors']}")
        set_expanded_range(disc, info, headers, LEVEL_RANGES.index("data"), first, count)
        return changes + notes + [f"level data expanded to {len(expanded)} bytes in virtual sectors"], problems, True
    runs = collision_scene.level_data_patches(data, core, index, level.index, compress)
    for (offset, payload), kind in zip(runs, ["index", "core"] if len(runs) == 2 else ["core"]):
        file = f"{name}/{kind}_{disc.version.key}.bin"
        (out / name).mkdir(parents=True, exist_ok=True)
        (out / file).write_bytes(payload)
        patches.append(f"{info['ranges']['data']['lba'] + offset // SECTOR}\t{len(payload) // SECTOR}\t{file}")
    return changes + notes, problems, True


# What the cutscene player and the port allow (docs/CUTSCENE_FORMAT.md in OpenRAC Editor; the SDK's
# OPENRAC_FIGURES_MAX, OPENRAC_PROPS_MAX and OPENRAC_JOINT_OFFSETS_MAX).
CUTSCENE_MAX_TICKS = 60 * 60 * 10      # ten minutes at 60 ticks a second
CUTSCENE_MAX_FIGURES = 8
CUTSCENE_MAX_PROPS = 256
CUTSCENE_MAX_JOINTS = 128
CUTSCENE_MAX_SOUND_BYTES = 64 << 20    # the voices and music it loads, all languages


def check_cutscene(doc: dict, source: Path, level_id: int | None = None, spawn_ids: set | None = None,
                   classes=None) -> tuple[list[str], list[str]]:
    """What is wrong with a .cutscene before it is packed: (errors, warnings) in words. An error
    leaves it out (it names another level, it is too long, it spawns a class the level does not
    load, it asks for more than the port has); a warning is something the player skips (a file not
    there, a moby not in the level, an actor that is not one). SOURCE is the folder its files are
    named from; SPAWN_IDS the level's mobys (None: not checked); CLASSES a set of the level's moby
    classes, or a function that gives it (called only when a cutscene spawns one)."""
    errors, warnings = [], []

    def items(value) -> list:
        """A list from the file, or none: a hand-written file may have anything there."""
        return value if isinstance(value, list) else []

    def part(value) -> dict:
        return value if isinstance(value, dict) else {}

    def name_of(value):
        """An id, a class or a moby as a set can hold it (anything else names nothing)."""
        return value if isinstance(value, (int, str)) and not isinstance(value, bool) else None
    if level_id is not None and isinstance(doc.get("level"), int) and doc["level"] != level_id:
        errors.append(f"it is for level {doc['level']}, not this level {level_id}")
    length = doc.get("length")
    if not isinstance(length, int) or isinstance(length, bool) or not 1 <= length <= CUTSCENE_MAX_TICKS:
        errors.append(f"its length {length!r} is not 1 to {CUTSCENE_MAX_TICKS} ticks")
    actors = [a for a in items(doc.get("actors")) if isinstance(a, dict)]
    ids = [name_of(a.get("id")) for a in actors]
    for actor_id in sorted({i for i in ids if ids.count(i) > 1}, key=str):
        errors.append(f"two actors are called {actor_id!r}")
    known = set(ids) | {"hero"}

    def file_there(path, what: str) -> Path | None:
        if not isinstance(path, str) or not path:
            return None
        try:
            found = (source / path).resolve()
            there = source.resolve() in found.parents and found.is_file()
        except (OSError, ValueError):   # a NUL in it, a name too long
            there = False
        if not there:
            warnings.append(f"{what}: {path} is not beside it")
            return None
        return found

    figures = props = 0
    for a in actors:
        name = a.get("id")
        src = part(a.get("source"))
        if "figure" in src:
            figures += 1
        if "prop" in src:
            props += 1
            file_there(src["prop"], f"actor {name!r}'s mesh")
        if "class" in src:
            have = classes() if callable(classes) else classes
            if have is not None and name_of(src["class"]) not in have:
                errors.append(f"actor {name!r} is a moby of class {src['class']}, which this level does not load")
        if "spawn_id" in src and spawn_ids is not None and name_of(src["spawn_id"]) not in spawn_ids and "class" not in src:
            warnings.append(f"actor {name!r} is the moby {src['spawn_id']}, which is not in this level")
        joints = set()
        for layer in items(a.get("layers")):
            if not isinstance(layer, dict):
                continue
            listed = [j for j in items(layer.get("joints")) if isinstance(j, int) and not isinstance(j, bool)]
            joints.update(listed)
            frames = file_there(layer.get("frames"), f"actor {name!r}'s layer {layer.get('name')!r}")
            if frames is not None and listed and frames.stat().st_size % (16 * len(listed)) != 0:
                warnings.append(f"actor {name!r}'s layer {layer.get('name')!r}: its frames are not "
                                f"{len(listed)} turns a tick")
        if len(joints) > CUTSCENE_MAX_JOINTS:
            errors.append(f"actor {name!r} turns {len(joints)} joints, more than {CUTSCENE_MAX_JOINTS}")
        pose = a.get("pose")
        if isinstance(pose, dict):
            count = pose.get("joints")
            frames = file_there(pose.get("frames"), f"actor {name!r}'s pose")
            if not isinstance(count, int) or isinstance(count, bool) or count <= 0:
                warnings.append(f"actor {name!r}'s pose has no joint count")
            elif frames is not None and frames.stat().st_size % (20 * count) != 0:
                warnings.append(f"actor {name!r}'s pose: its frames are not {count} joints a frame")
        for ride in items(a.get("ride")):
            if isinstance(ride, dict) and "actor" in ride and name_of(ride["actor"]) not in known:
                warnings.append(f"actor {name!r} rides on {ride['actor']!r}, who is not an actor")
        for key in items(a.get("look_at")):
            target = key.get("target") if isinstance(key, dict) else None
            if isinstance(target, dict) and "actor" in target and name_of(target["actor"]) not in known:
                warnings.append(f"actor {name!r} looks at {target['actor']!r}, who is not an actor")
    if figures > CUTSCENE_MAX_FIGURES:
        errors.append(f"{figures} figures, more than the port's {CUTSCENE_MAX_FIGURES}")
    if props > CUTSCENE_MAX_PROPS:
        errors.append(f"{props} meshes, more than the port's {CUTSCENE_MAX_PROPS}")
    for key in items(part(doc.get("camera")).get("keys")):
        for end in ("eye", "look_at"):
            point = key.get(end) if isinstance(key, dict) else None
            if isinstance(point, dict) and "actor" in point and name_of(point["actor"]) not in known:
                warnings.append(f"the camera's {end} at tick {key.get('t')} is on {point['actor']!r}, who is not an actor")
    sound_bytes = 0
    for sound in items(doc.get("sounds")):
        if not isinstance(sound, dict):
            continue
        if sound.get("actor") not in (None, "hero") and name_of(sound["actor"]) not in known:
            warnings.append(f"a sound at tick {sound.get('t')} is on {sound['actor']!r}, who is not an actor")
        files = sound.get("file")
        for path in files.values() if isinstance(files, dict) else [files]:
            found = file_there(path, f"the sound at tick {sound.get('t')}")
            sound_bytes += found.stat().st_size if found is not None else 0
    if sound_bytes > CUTSCENE_MAX_SOUND_BYTES:
        errors.append(f"its sounds are {sound_bytes >> 20} MiB, more than {CUTSCENE_MAX_SOUND_BYTES >> 20} MiB")
    trigger = part(doc.get("trigger"))
    if trigger.get("kind") in ("talk", "pickup") and spawn_ids is not None and name_of(trigger.get("spawn_id")) not in spawn_ids:
        warnings.append(f"its {trigger['kind']} trigger is the moby {trigger.get('spawn_id')}, which is not in this level")
    for event in items(doc.get("events")):
        if (isinstance(event, dict) and event.get("do") in ("moby_hide", "moby_kill") and spawn_ids is not None
                and name_of(event.get("spawn_id")) not in spawn_ids):
            warnings.append(f"the {event['do']} at tick {event.get('t')} is the moby {event.get('spawn_id')}, "
                            "which is not in this level")
    return errors, warnings


def pack_cutscenes(scene_dir: Path, out_dir: Path, level_id: int | None = None, spawn_ids: set | None = None,
                   classes=None) -> tuple[list[str], list[str]]:
    """Copies the level's cutscenes (OpenRAC Editor writes them to <scene folder>/cutscenes/ on
    save) with the files beside them (voices, music, meshes, layers' frames) to
    <out>/level_NN/cutscenes/, where the editor-runtime mod reads them. Each is checked first
    (check_cutscene). Returns (changes, problems) in words; a .cutscene that is not one, or has an
    error, is left out."""
    source = scene_dir / "cutscenes"
    if not source.is_dir():
        return [], []
    changes, problems, files = [], [], []
    for file in sorted(source.rglob("*")):
        if file.is_symlink() or not file.is_file() or file.name.startswith(".") or file.suffix == ".import":
            continue
        if file.suffix == ".cutscene":
            try:
                doc = json.loads(file.read_text(encoding="utf-8"))
                if not isinstance(doc, dict) or doc.get("format") != "openrac-cutscene":
                    raise ValueError("no \"format\": \"openrac-cutscene\"")
            except (OSError, ValueError, AttributeError) as error:
                problems.append(f"cutscene {file.name}: {error}")
                continue
            errors, warnings = check_cutscene(doc, file.parent, level_id, spawn_ids, classes)
            problems += [f"cutscene {file.stem}: {w}" for w in warnings]
            if errors:
                problems += [f"cutscene {file.stem}: {e}; left out" for e in errors]
                continue
            changes.append(f"cutscene {file.stem}")
        files.append(file)
    if not changes:
        return [], problems
    import shutil
    target = out_dir / "cutscenes"
    for file in files:
        copy = target / file.relative_to(source)
        copy.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(file, copy)
    return changes, problems


def pack(disc, survey: dict, project: Path, out: Path,
         moby_names: dict[int, tuple[Path, list[str]]] | None = None) -> tuple[int, int]:
    """Writes OUT from the scenes of PROJECT, for DISC's version. Returns (levels packed, mobys changed).
    MOBY_NAMES, when given, gets each level whose mobys were packed: its scene's folder and its
    mobys' node names in the packed table's order (moby_states.py works out their start states)."""
    import scenery          # it reads the scene with this module's reader
    patches, virtual, report, packed, total = [], [], [], 0, 0
    headers: dict[int, bytearray] = {}
    next_virtual = [0x10000000]

    def scene_of(info: dict) -> Path | None:
        """The level's scene: one project with every level, or a project per level (`godot --split`)."""
        name = f"level_{info['id']:02}"
        return next((s for s in (project / "levels" / name / f"{name}.tscn",
                                 project / name / "levels" / name / f"{name}.tscn") if s.is_file()), None)
    found = [(info, scene) for info in survey["levels"] if (scene := scene_of(info)) is not None]
    for info, scene in progress_line.counted("pack", found, lambda item: item[0]["id"]):
        name = f"level_{info['id']:02}"
        if disc.version.key == "pal":
            metadata = scene.with_name("level.json")
            try:
                source = json.loads(metadata.read_text()).get("gameplay_source")
            except (OSError, ValueError, AttributeError):
                source = None
            if source != disc.version.gameplay:
                raise FormatError(f"level {info['id']:02}: this PAL project was read from the 50 Hz gameplay "
                                  f"copy ({source or 'gameplay_pal'}); the game plays {disc.version.gameplay}. "
                                  "Extracting the levels again backs it up and carries its edits over (carry.py).",
                                  code="project-outdated-gameplay")
        place = info["ranges"][disc.version.gameplay]
        stored = disc.sectors(place["lba"], place["sectors"])
        gameplay = decoded(stored)
        text = scene.read_text()
        scene_nodes, _ = gamedata.parse_scene(text)
        game_frame = scene_nodes.get("Game")
        # A scene without the node (an older project, or one written by hand) has nothing to keep.
        if game_frame is not None and not same(gamedata.transform_of(game_frame), GAME_FRAME):
            raise FormatError(
                f"level {info['id']:02}: keep the Game node's extracted transform; it converts Godot's "
                "coordinates to the game's and cannot be packed as a level edit",
                code="pack-incomplete"
            )
        validate_packed_visibility(scene_nodes, info["id"])
        import static_assets
        try:
            static_instances = static_assets.placements(scene.parents[2], text)
        except (ValueError, KeyError, IndexError) as error:
            raise FormatError(f'Static import placement: {error}', code="static-import") from None
        authored_classes = {n["class_id"] for n in scene_mobys(text) if n["class_id"] is not None}
        # The level's own classes only when the scene has any to check against them.
        original_classes = {m["class_id"] for m in moby_instances(gameplay)} if authored_classes else set()
        available_classes = None
        if authored_classes - original_classes:
            target_level = load_level(disc, info)
            available_classes = {struct.unpack_from("<i", entry, 4)[0]
                                 for entry in target_level.table(0x18, 32)}
        # Imported classes: another level's class, which runs in its own level's program
        # (foreign_classes.py).
        import foreign_classes
        wanted, foreign_problems = foreign_classes.wanted(text)
        donors, blobs, sources = {}, {}, {}
        owner: dict[int, int] = {}     # each imported class: the placed class it came with
        for class_id, there in wanted.items():
            try:
                if available_classes is not None and class_id in available_classes:
                    raise ValueError(f"class {class_id} is this level's own; it needs no rc1_foreign_level")
                if disc.version.key not in foreign_classes.DISCS:
                    raise ValueError(f"class {class_id}: imported classes run on Ratchet & Clank's discs only")
                foreign_classes.check(info["id"], there, class_id)
                if there not in sources:     # each level read once, for all its classes
                    source = next((i for i in survey["levels"] if i["id"] == there), None)
                    if source is None:
                        raise ValueError(f"class {class_id}: this disc has no level {there:02}")
                    source_place = source["ranges"][disc.version.gameplay]
                    sources[there] = (decoded(disc.sectors(source_place["lba"], source_place["sectors"])),
                                      load_level(disc, source))
                source_gameplay, source_level = sources[there]
                found = foreign_classes.donor(there, source_gameplay, class_id)
                if found is None:
                    raise ValueError(f"class {class_id}: level {there:02} places no moby of it to copy")
                blobs[class_id] = (there, foreign_classes.class_blob(source_level, class_id))
                donors[class_id] = [found]
                owner[class_id] = class_id
            except (ValueError, FormatError) as error:
                foreign_problems.append(f"imported {error}; its mobys are not packed")
        # What each one's links name in its level comes with it: a class this level has runs as
        # its own, one it lacks is imported from that level too (none the scene imports elsewhere).
        own = available_classes if available_classes is not None else \
            {struct.unpack_from("<i", entry, 4)[0] for entry in load_level(disc, info).table(0x18, 32)} \
            if donors else set()
        for class_id, group in list(donors.items()):
            there = group[0].level
            source_gameplay, source_level = sources[there]
            for member in foreign_classes.linked(there, source_gameplay, group[0])[1:]:
                cid = member.class_id
                if cid not in own and cid not in blobs:
                    if wanted.get(cid, there) != there:
                        continue
                    try:
                        blobs[cid] = (there, foreign_classes.class_blob(source_level, cid))
                        owner[cid] = class_id
                    except ValueError:
                        continue
                if cid in blobs and blobs[cid][0] != there:
                    continue
                group.append(member)
        # And the classes its update can make that this level lacks (spawns.py: a turret's shots).
        import spawns
        made = spawns.table()
        for class_id, group in list(donors.items()):
            there = group[0].level
            _, source_level = sources[there]
            for member_class in sorted({member.class_id for member in group}):
                for cid in sorted(spawns.made_by(member_class, there, made)):
                    if cid in own or cid in blobs or wanted.get(cid, there) != there:
                        continue
                    try:
                        blobs[cid] = (there, foreign_classes.class_blob(source_level, cid))
                        owner[cid] = class_id
                    except ValueError:
                        pass
        # The level has 224 class slots; what its own classes leave is all the imports get. A
        # placed class whose classes (its own, its links', what it makes) do not all fit is not
        # imported: without its parts or shots it would not run as it does there.
        room = max(foreign_classes.CLASS_SLOTS - len(own), 0) if blobs else 0
        taken: set[int] = set()
        for class_id in list(donors):
            needs = {c for c in blobs if owner.get(c) == class_id} - taken
            if len(taken) + len(needs) <= room:
                taken |= needs
                continue
            del donors[class_id]
            foreign_problems.append(
                f"imported class {class_id} needs {len(needs)} more moby classes and only "
                f"{room - len(taken)} fit in this level; its mobys are not packed")
        for cid in [c for c in blobs if c not in taken]:
            del blobs[cid]
        names: list[str] = []
        moby_spawn_ids: dict[str, int] = {}
        placed: list = []
        edited, changes, problems = pack_level(info["id"], gameplay, text, available_classes, names, moby_spawn_ids,
                                               donors, placed)
        problems += foreign_problems
        changes += foreign_classes.write(out / name, blobs) if donors else []

        if not donors:
            foreign_classes.write(out / name, {})
        # Each imported moby's own level's tables, moved to where it stands (foreign_classes.world).
        changes += foreign_classes.write_worlds(
            out / name, [(copies, sources[copies[0][1].level][0]) for copies in placed])
        if edited != gameplay and len(edited) > EXPANDED_RAM_BYTES:
            raise FormatError(f"level {info['id']:02}: decoded gameplay data is {len(edited)} bytes; it cannot "
                              "fit in the 256 MiB expanded guest RAM arena", code="level-too-big")
        # Ties and shrubs: the port draws them from the extractor's files, so they are packed as
        # those are (scenery.py), beside the gameplay file.
        placements, scenery_changes, scenery_problems = None, [], []
        if scenery.edited(gameplay, text):
            placements, scenery_changes, scenery_problems = scenery.pack_scenery(load_level(disc, info), text)
        # Collision, the level's and its moby classes': the level data's core (collision_scene.py,
        # moby_collision_scene.py), only when the scenes' differ.
        collision_changes, collision_problems, wrote = pack_collision(disc, info, scene, text, out,
                                                                      patches, virtual, headers, next_virtual)
        if any(solid for _, _, _, solid in static_instances) and collision_problems:
            raise FormatError('Static import collision could not be packed: ' + '; '.join(collision_problems),
                              code="static-import")
        try:
            static_bake = static_assets.write_packed(scene.parents[2], scene.parent, text, out / name)
        except (ValueError, KeyError, IndexError) as error:
            raise FormatError(f'Static import geometry: {error}', code="static-import") from None
        try:
            animated_count = static_assets.write_animated(scene.parents[2], text, out / name)
        except (OSError, ValueError, KeyError, IndexError) as error:
            raise FormatError(f'Animated prop export: {error}', code="static-import") from None
        scenery_changes += collision_changes
        scenery_problems += collision_problems
        if static_bake is not None:
            scenery_changes.append(f"static imports: {static_bake['instances']} instance(s) baked into terrain")
            if not changes and placements is None and not wrote and not animated_count:
                packed += 1
                total += static_bake['instances']
        if animated_count:
            scenery_changes.append(f"animated props: {animated_count} spinning visual instance(s)")
            total += animated_count
            if not changes and placements is None and not wrote and static_bake is None:
                packed += 1
        # Skins: mobys drawn as another level's or game's model (skins.py), beside the placements.
        import skins
        skin_changes, skin_problems = skins.write_skins(scene.parents[2], text, out / name, moby_spawn_ids)
        scenery_changes += skin_changes
        scenery_problems += skin_problems
        if skin_changes and not changes and placements is None and not wrote and static_bake is None \
                and not animated_count:
            packed += 1
        # Behaviours: mobys the editor-runtime mod moves (behaviours.py), beside the placements.
        import behaviours
        behaviour_changes, behaviour_problems = behaviours.write_behaviours(text, out / name, moby_spawn_ids)
        scenery_changes += behaviour_changes
        scenery_problems += behaviour_problems
        if behaviour_changes and not changes and placements is None and not wrote and static_bake is None \
                and not animated_count and not skin_changes:
            packed += 1
        # Cutscenes: files the editor-runtime mod reads from the pack, beside the level's.
        level_classes = []

        def classes_loaded() -> set:
            if not level_classes:
                level_classes.append({struct.unpack_from("<i", entry, 4)[0]
                                      for entry in load_level(disc, info).table(0x18, 32)})
            return level_classes[0]
        spawn_ids = None
        if (scene.parent / "cutscenes").is_dir():
            try:
                spawn_ids = {m["fields"]["spawn_id"] for m in moby_instances(gameplay)}
                spawn_ids |= {n["meta"]["spawn_id"] for n in scene_mobys(text)
                              if isinstance(n["meta"].get("spawn_id"), int)}
            except FormatError:
                spawn_ids = None    # a gameplay file it cannot read: the mobys are not checked
        cutscene_changes, cutscene_problems = pack_cutscenes(scene.parent, out / name, info["id"], spawn_ids,
                                                             classes_loaded)
        scenery_problems += cutscene_problems
        if cutscene_changes:
            scenery_changes += cutscene_changes
            total += len(cutscene_changes)
            if not changes and placements is None and not wrote and static_bake is None and not animated_count:
                packed += 1
        if wrote:
            packed += 0 if changes or placements is not None else 1
            total += len(collision_changes)
        if changes or problems or scenery_changes or scenery_problems:
            report.append(f"level {info['id']:02}")
            report += [f"  {line}" for line in changes + scenery_changes]
            report += [f"  not packed: {line}" for line in problems + scenery_problems]
        if placements is not None:
            (out / name).mkdir(parents=True, exist_ok=True)
            (out / name / "placements.json").write_text(json.dumps(placements, separators=(",", ":")) + "\n")
            total += len(scenery_changes)
            packed += 0 if changes else 1
        if not changes:
            continue
        stream = compress(edited, stored[:16])
        if decoded(stream) != edited:
            raise FormatError(f"level {info['id']}: the packed file does not read back", code="pack-internal")
        room = place["sectors"] * SECTOR
        if len(stream) > room:
            storage = "stored in expanded virtual sectors"
        elif len(edited) > VANILLA_RAM_BYTES:
            storage = "stored in virtual sectors to select the expanded RAM arena"
        else:
            storage = "fits disc slot"
        report.append(
            f"  gameplay size: {len(edited)} bytes decoded; {len(stream)} compressed, "
            f"disc slot {room} bytes ({storage})"
        )
        if len(stream) <= room and len(edited) <= VANILLA_RAM_BYTES:
            file = f"{name}/gameplay_{disc.version.key}.wad"
            (out / file).parent.mkdir(parents=True, exist_ok=True)
            (out / file).write_bytes(stream + bytes(room - len(stream)))
            patches.append(f"{place['lba']}\t{place['sectors']}\t{file}")
        else:
            # Store expanded content in the mod's virtual disc and redirect
            # only this level's header. The user's disc is never rewritten.
            count = -(-len(stream) // SECTOR)
            first = virtual_range(next_virtual, count)
            file = f"{name}/gameplay_{disc.version.key}.wad"
            (out / file).parent.mkdir(parents=True, exist_ok=True)
            (out / file).write_bytes(stream + bytes(count * SECTOR - len(stream)))
            range_index = LEVEL_RANGES.index(disc.version.gameplay)
            virtual.append(f"{first}\t{count}\t{file}\t{info['id']}\t{range_index}\t"
                           f"{place['lba']}\t{place['sectors']}")
            set_expanded_range(disc, info, headers, range_index, first, count)
            report.append(f"  gameplay stream expanded to {len(stream)} bytes in virtual sectors")
        packed += 1
        total += len(changes)
        if moby_names is not None:
            moby_names[info["id"]] = (scene.parent, names)
    out.mkdir(parents=True, exist_ok=True)
    for level, header in sorted(headers.items()):
        info = survey["levels"][level]
        file = f"level_{level:02}/header_{disc.version.key}.bin"
        (out / file).parent.mkdir(parents=True, exist_ok=True)
        (out / file).write_bytes(header + bytes(-len(header) % SECTOR))
        patches.append(f"{info['header_lba']}\t{-(-LEVEL_HEADER_SIZE // SECTOR)}\t{file}")
    (out / "patches.tsv").write_text("# first sector\tsectors\tfile (beside this list)\n" + "".join(p + "\n" for p in patches))
    (out / "virtual.tsv").write_text("# first sector\tsectors\tfile (beside this list)\n" + "".join(p + "\n" for p in virtual))
    (out / "changes.txt").write_text("\n".join(report) + "\n" if report else "No level differs from the disc.\n")
    (out / "packed.json").write_text(json.dumps({"format": 1, "game": "rac1", "version": disc.version.key}) + "\n")
    return packed, total
