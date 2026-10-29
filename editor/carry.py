"""Carry a player's edits from an outdated level project onto a fresh one.

When the editor changes what a project is made from (a new project schema, or the PAL projects
read from the 60 Hz gameplay copy since the 60 Hz fix), `extract.py godot` moves the old level
project to `<projects>-backups/<time>/level_NN` and writes a fresh one (extract.update_godot).
This module then looks at the old project the way the packer does (pack.py): it reads the
gameplay copy the old project was made from, sees what the player changed, and makes the same
changes in the fresh scene, so that the player only packs again.

What is carried, for each moby (the level's objects):

- moved, turned or scaled: the node's transform;
- another class: the node's class scene;
- values (`rc1_*` metadata, the pvars `rc1_pv_*`, the links `rc1_link_*`): each value the old
  node has that differs from what the old gameplay copy says (a value the old node does not
  have is left as the fresh project has it: an older editor did not write every value);
- deleted: the node goes;
- added (a copy made in Godot, or a class scene dropped in): the node, with what it is an
  instance of, after the level's mobys.

What is not carried, and listed so the player can redo it: edits of cameras, volumes, sounds,
paths, grind paths, moby groups and level settings; ties and shrubs; the level's collision and
the moby classes' collision; imported static models. Matching a moby of the old project to the
fresh one is by its place in the level's table when both tables list the same classes in the same
order (true of the two gameplay copies of every PAL level), else by class and spawn number.

    carry_level(old_scene, old_gameplay, new_scene, new_gameplay) -> (new scene text, report)
"""

from __future__ import annotations

import json
import re
from pathlib import Path

import gamedata
import pack
from formats import FormatError
from mobys import moby_class_names, moby_instances, node_name

# A scene's section heads: [gd_scene], [ext_resource], [sub_resource], [node], [connection].
NODE_HEAD = re.compile(r'^\[node name="([^"]+)"(?P<rest>.*)\]$')
PARENT = re.compile(r'parent="([^"]*)"')
INSTANCE = re.compile(r'instance=ExtResource\("([^"]+)"\)')
EXT = re.compile(r'^\[ext_resource [^\]]*path="([^"]+)"[^\]]*id="([^"]+)"')
MOBYS = "Game/Mobys"
# Metadata that describes a moby's class or place, never carried as an edit.
DESCRIPTIVE = ("index", "class", "class_name", "source", "update")
# What the report calls each kind of node that is not carried.
KINDS = (("Game/Cameras", "cameras"), ("Game/Sounds", "sounds"), ("Game/Volumes", "volumes"),
         ("Game/Paths", "paths"), ("Game/GrindPaths", "grind paths"), ("Game/LevelSettings", "level settings"),
         ("Game/MobyGroups", "moby groups"))


class Block:
    """One section of a scene: its head line and its properties, in order (a value may span lines)."""

    def __init__(self, head: str):
        self.head = head
        self.props: list[tuple[str, str]] = []
        self.extra: list[str] = []          # lines that are not properties (kept as they are)
        self.changed = False
        self.text: str | None = None        # the section as read, written back unless changed
        self.blank = True                   # a blank line after it

    @property
    def node(self) -> tuple[str, str | None] | None:
        """(name, parent) of a node section."""
        found = NODE_HEAD.match(self.head)
        if not found:
            return None
        parent = PARENT.search(found.group("rest"))
        return found.group(1), parent.group(1) if parent else None

    @property
    def path(self) -> str | None:
        node = self.node
        if node is None:
            return None
        name, parent = node
        return "." if parent is None else (name if parent == "." else f"{parent}/{name}")

    def get(self, key: str) -> str | None:
        return next((v for k, v in self.props if k == key), None)

    def set(self, key: str, value: str) -> None:
        for i, (k, _) in enumerate(self.props):
            if k == key:
                self.props[i] = (key, value)
                break
        else:
            self.props.append((key, value))
        self.changed = True

    def render(self) -> str:
        if not self.changed and self.text is not None:
            return self.text
        lines = [self.head] + [f"{k} = {v}" for k, v in self.props] + self.extra + ([""] if self.blank else [])
        return "\n".join(lines) + "\n"


def read(text: str) -> list[Block]:
    """A .tscn as its sections, the file's first line included as a section of its own."""
    blocks: list[Block] = []
    current: Block | None = None
    raw: list[str] = []
    lines = text.splitlines()
    i = 0

    def close():
        if current is not None:
            current.text = "\n".join(raw) + "\n"
            current.blank = bool(raw) and raw[-1] == ""

    while i < len(lines):
        line = lines[i]
        i += 1
        if line.startswith("["):
            close()
            current = Block(line)
            raw = [line]
            blocks.append(current)
            continue
        if current is None:
            continue
        raw.append(line)
        prop = gamedata.PROPERTY.match(line)
        if not prop:
            if line.strip():
                current.extra.append(line)
            continue
        key, value = prop.groups()
        level = gamedata.depth(value)
        while level > 0 and i < len(lines):
            value += "\n" + lines[i]
            raw.append(lines[i])
            level += gamedata.depth(lines[i])
            i += 1
        current.props.append((key, value))
    close()
    return blocks


def write(blocks: list[Block]) -> str:
    """The scene again: the sections as read, the changed ones written anew."""
    return "".join(block.render() for block in blocks)


def ext_resources(blocks: list[Block]) -> dict[str, str]:
    """ext_resource id -> path."""
    found = {}
    for block in blocks:
        m = EXT.match(block.head)
        if m:
            found[m.group(2)] = m.group(1)
    return found


def class_of(path: str | None) -> int | None:
    found = pack.CLASS_SCENE.search(path or "")
    return int(found.group(2)) if found and found.group(1) == "moby" else None


def index_of(block: Block) -> int | None:
    value = block.get("metadata/rc1_index")
    try:
        return int(value) if value is not None else None
    except ValueError:
        return None


def under(block: Block, path: str) -> bool:
    """BLOCK is a node below PATH (its child, grandchild, ...)."""
    node = block.node
    return node is not None and node[1] is not None and (node[1] == path or node[1].startswith(path + "/"))


def match_tables(old: list[dict], new: list[dict]) -> dict[int, int]:
    """Old moby index -> fresh moby index: by place when both tables list the same classes in the
    same order, else by class and spawn number where that is unique."""
    if len(old) == len(new) and all(a["class_id"] == b["class_id"] for a, b in zip(old, new)):
        return {i: i for i in range(len(old))}
    keyed: dict[tuple, list[int]] = {}
    for m in new:
        keyed.setdefault((m["class_id"], m["fields"].get("spawn_id")), []).append(m["index"])
    out = {}
    for m in old:
        found = keyed.get((m["class_id"], m["fields"].get("spawn_id")), [])
        if len(found) == 1:
            out[m["index"]] = found[0]
    return out


def moby_value_edits(node: dict, block: Block, original: dict, pristine_extra: dict) -> dict[str, str]:
    """The metadata keys of an old moby node that differ from its gameplay copy, with their text."""
    edits = {}
    for key, value in node["meta"].items():
        if key in DESCRIPTIVE:
            continue
        text = block.get(f"metadata/rc1_{key}")
        if text is None:
            continue
        if gamedata.owns(key):
            first = pristine_extra.get(f"metadata__rc1_{key}")
            if first is None:
                edits[key] = text
                continue
            try:
                same = gamedata.same_value(gamedata.parse_value(text), gamedata.parse_value(str(first)))
            except FormatError:
                same = text.strip() == str(first).strip()
            if not same:
                edits[key] = text
        else:
            if key not in original["fields"] or not pack.same_value(value, original["fields"][key]):
                edits[key] = text
    return edits


def carry_level(old_scene: str, old_gameplay: bytes, new_scene: str, new_gameplay: bytes) -> tuple[str, dict]:
    """The fresh scene NEW_SCENE with the edits of OLD_SCENE (made from OLD_GAMEPLAY) carried over,
    and a report: {"carried": [lines], "not_carried": [lines], "mobys": n}."""
    old_blocks, new_blocks = read(old_scene), read(new_scene)
    old_ext, new_ext = ext_resources(old_blocks), ext_resources(new_blocks)
    old_orig, new_orig = moby_instances(old_gameplay), moby_instances(new_gameplay)
    places = match_tables(old_orig, new_orig)
    pristine_extra = gamedata.moby_metadata(old_gameplay)
    nodes = {n["name"]: n for n in pack.scene_mobys(old_scene)}
    carried: list[str] = []
    not_carried: list[str] = []

    # The fresh scene's moby nodes, by index.
    fresh: dict[int, Block] = {}
    for block in new_blocks:
        node = block.node
        if node and node[1] == MOBYS:
            index = index_of(block)
            if index is not None and (index not in fresh or node[0].endswith(f"_{index:04}")):
                fresh[index] = block

    # The old nodes that are each moby (pack.pack_level's rule: a copy keeps its source's index,
    # the node still named after its place is the moby).
    owner: dict[int, Block] = {}
    old_mobys = [b for b in old_blocks if b.node and b.node[1] == MOBYS]
    for block in old_mobys:
        index = index_of(block)
        if index is None or not 0 <= index < len(old_orig):
            continue
        named = block.node[0].endswith(f"_{index:04}")
        if index not in owner or (named and not owner[index].node[0].endswith(f"_{index:04}")):
            owner[index] = block

    def ext_id(path: str) -> str:
        """The fresh scene's ext_resource for PATH, added when it has none."""
        for rid, p in new_ext.items():
            if p == path:
                return rid
        rid = f"carried_{len(new_ext)}"
        new_ext[rid] = path
        head = f'[ext_resource type="PackedScene" path="{path}" id="{rid}"]'
        at = max((i for i, b in enumerate(new_blocks) if b.head.startswith("[ext_resource")), default=1)
        block = Block(head)
        block.changed = True
        block.blank = False
        new_blocks.insert(at, block)
        return rid

    # Edited and deleted mobys.
    class_names = moby_class_names()
    deleted = 0
    for index, original in enumerate(old_orig):
        block = owner.get(index)
        target = places.get(index)
        name = block.node[0] if block else None
        if target is None or target not in fresh:
            if block is not None:
                not_carried.append(f"{name}: no matching object in the new level")
            continue
        dest = fresh[target]
        name = name or dest.node[0]
        if block is None:
            deleted += 1
            new_blocks.remove(dest)
            path = dest.path
            new_blocks[:] = [b for b in new_blocks if not under(b, path)]
            carried.append(f"{name}: deleted")
            continue
        node = nodes.get(block.node[0])
        if node is None:
            continue
        edits = []
        if not pack.same(node["transform"], pack.scene_transform(original["matrix"])):
            dest.set("transform", block.get("transform") or "")
            edits.append("moved")
        cid = class_of(old_ext.get((INSTANCE.search(block.head) or [None, None])[1] or ""))
        if cid is not None and cid != original["class_id"]:
            rid = ext_id(old_ext[INSTANCE.search(block.head).group(1)])
            dest.head = INSTANCE.sub(f'instance=ExtResource("{rid}")', dest.head)
            dest.changed = True
            edits.append("class")
        values = moby_value_edits(node, block, original, pristine_extra.get(index, {}))
        for key, text in values.items():
            dest.set(f"metadata/rc1_{key}", text)
        if values:
            edits.append("values")
        if edits:
            carried.append(f"{block.node[0]}: {', '.join(edits)}")
    if old_orig and deleted > len(old_orig) // 2:
        raise FormatError(f"{deleted} of {len(old_orig)} objects would be deleted; the old scene was not read "
                          "as expected, so nothing was carried", code="carry-failed")

    # Added mobys: their sections (and their children's) after the fresh scene's mobys.
    names = {b.path for b in new_blocks if b.node}
    last = max((i for i, b in enumerate(new_blocks) if b.node and (b.node[1] or "").startswith(MOBYS)), default=None)
    for block in old_mobys:
        index = index_of(block)
        if index is not None and owner.get(index) is block:
            continue
        name = block.node[0]
        family = [block] + [b for b in old_blocks if under(b, block.path)]
        if any("SubResource(" in b.render() for b in family):
            not_carried.append(f"{name}: added with resources of its own")
            continue
        copies = []
        for member in family:
            copy = Block(member.head)
            copy.props = list(member.props)
            copy.extra = list(member.extra)
            copy.changed = True
            found = INSTANCE.search(copy.head)
            if found:
                path = old_ext.get(found.group(1))
                if path is None:
                    break
                copy.head = INSTANCE.sub(f'instance=ExtResource("{ext_id(path)}")', copy.head)
            copies.append(copy)
        else:
            if index is not None:
                if index in places:
                    copies[0].set("metadata/rc1_index", str(places[index]))
                else:
                    not_carried.append(f"{name}: its source object is not in the new level")
                    continue
            new_name = name
            while f"{MOBYS}/{new_name}" in names:
                new_name += "_carried"
            if new_name != name:
                for copy in copies:
                    copy.head = copy.head.replace(f'name="{name}"', f'name="{new_name}"', 1)
                    copy.head = copy.head.replace(f'parent="{MOBYS}/{name}', f'parent="{MOBYS}/{new_name}', 1)
            names.add(f"{MOBYS}/{new_name}")
            last = (max((i for i, b in enumerate(new_blocks) if b.node and (b.node[1] or "").startswith(MOBYS)),
                        default=len(new_blocks) - 1) if last is None else last)
            for copy in copies:
                last += 1
                new_blocks.insert(last, copy)
            carried.append(f"{new_name}: added")
            continue
        not_carried.append(f"{name}: added from a scene the new level does not have")

    # What else the player changed, as the packer sees it: listed, not carried.
    moby_names = {b.node[0] for b in old_mobys} | {node_name(m["class_id"], m["index"], class_names) for m in old_orig}
    try:
        _, changes, _ = pack.pack_level(0, old_gameplay, old_scene)
    except (FormatError, ValueError, KeyError, IndexError):
        changes = []
    for line in changes:
        name = line.split(":", 1)[0]
        if name in moby_names or line.startswith(("moby ", "moby groups")):
            continue
        not_carried.append(line)
    text = write(new_blocks)
    return text, {"carried": carried, "not_carried": not_carried, "mobys": len(carried)}


def other_edits(project: Path, level_dir: Path, scene: str, gameplay: bytes) -> list[str]:
    """What the old project changed that is not in its level scene's gameplay nodes: ties and
    shrubs, collision, class collision, imported static models (each said once)."""
    import collision_scene
    import moby_collision_scene
    import scenery
    import static_assets
    found = []
    try:
        if scenery.edited(gameplay, scene):
            found.append("ties and shrubs")
    except (FormatError, ValueError, KeyError, IndexError):
        pass
    try:
        if collision_scene.scene_collision(project, level_dir, scene) is not None:
            found.append("the level's collision")
    except (FormatError, ValueError, KeyError, IndexError, OSError):
        pass
    try:
        if not moby_collision_scene.unchanged_scenes(level_dir):
            found.append("the collision of object classes")
    except (OSError, ValueError, KeyError):
        pass
    try:
        if static_assets.placements(project, scene):
            found.append("imported static models")
    except (FormatError, ValueError, KeyError, IndexError, OSError):
        pass
    return found


REPORT = "carried.json"


def carry_project(old_project: Path, old_level_dir: Path, new_level_dir: Path, old_gameplay: bytes,
                  new_gameplay: bytes, backup: Path) -> dict:
    """Carries one level's edits from the backed-up project into the fresh one, writes the report
    beside the fresh scene (carried.json, which the launcher reads) and returns it."""
    name = new_level_dir.name
    old_scene = (old_level_dir / f"{name}.tscn").read_text()
    new_file = new_level_dir / f"{name}.tscn"
    try:
        text, report = carry_level(old_scene, old_gameplay, new_file.read_text(), new_gameplay)
    except FormatError as exc:
        report = {"carried": [], "not_carried": [str(exc)], "mobys": 0, "failed": True}
        text = None
    report["not_carried"] += [f"{what}: redo by hand" for what in
                              other_edits(old_project, old_level_dir, old_scene, old_gameplay)]
    if text is not None and report["carried"]:
        try:
            pack.pack_level(0, new_gameplay, text)
        except (FormatError, ValueError, KeyError, IndexError) as exc:
            # The carried scene would not pack: the fresh one stays as it was written.
            report = {"carried": [], "not_carried": report["not_carried"] + [f"the carried edits would not pack: {exc}"],
                      "mobys": 0, "failed": True}
            text = None
        if text is not None:
            new_file.write_text(text)
    report["backup"] = str(backup)
    report["level"] = int(name.split("_")[1])
    (new_level_dir / REPORT).write_text(json.dumps(report, indent=1) + "\n")
    return report
