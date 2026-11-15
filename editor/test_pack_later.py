"""pack_later.pack_level on a made-up Going Commando gameplay file and scene."""

import math
import struct
import unittest

import games
from godot import transform
from mobys import moby_instances
from pack_later import pack_level

GC = games.RAC2
RECORD = struct.Struct(GC.moby.layout)


def gameplay(records: list[dict]) -> bytes:
    start, size = 0x100, GC.moby.size
    out = bytearray(start + 0x10 + len(records) * size)
    struct.pack_into("<I", out, GC.mobys, start)
    struct.pack_into("<i", out, start, len(records))
    for i, fields in enumerate(records):
        record = {"size": size, **GC.moby.usual, "spawn_id": i, "group": -1, "pvar_index": -1, "class_id": 500,
                  "scale": 1.0, "x": 0.0, "y": 0.0, "z": 0.0, "rx": 0.0, "ry": 0.0, "rz": 0.0,
                  "r": 10, "g": 20, "b": 30, **fields}
        RECORD.pack_into(out, start + 0x10 + i * size, *(record[f] for f in GC.moby.fields))
    return bytes(out)


def scene(data: bytes, edit=None, skip=()) -> str:
    """The moby nodes as the extractor writes them; EDIT changes the lines of one node, SKIP leaves nodes out."""
    lines = ['[ext_resource type="PackedScene" path="res://levels/level_00/mobys/moby_500.tscn" id="moby_500"]', ""]
    for p in moby_instances(data, GC):
        if p["index"] in skip:
            continue
        node = [f'[node name="Moby_{p["index"]:04}" parent="Game/Mobys" instance=ExtResource("moby_{p["class_id"]}")]',
                "rotation_order = 0", f"transform = {transform(p['matrix'])}", f"metadata/rc1_index = {p['index']}"]
        node += [f"metadata/rc1_{k} = {'inf' if v == math.inf else v}" for k, v in p["fields"].items()]
        if edit:
            node = edit(p["index"], node)
        lines += node + [""]
    return "\n".join(lines)


class PackLater(unittest.TestCase):
    def setUp(self):
        self.data = gameplay([{"x": 1.5, "y": -2.25, "z": 3.0, "rz": 0.7, "scale": 2.0},
                              {"x": 10.0, "rx": 0.3, "rooted_distance": math.inf, "mission": 4},
                              {"x": -5.0, "y": 0.1}])

    def test_untouched_scene_changes_nothing(self):
        self.assertEqual(pack_level(self.data, scene(self.data), GC), (self.data, [], []))

    def test_a_moved_moby_changes_only_its_record(self):
        def move(index, node):
            if index == 1:
                node[2] = "transform = Transform3D(0, -3, 0, 3, 0, 0, 0, 0, 3, 7, 8, 9)"
            return node
        out, changes, problems = pack_level(self.data, scene(self.data, move), GC)
        self.assertEqual((changes, problems), (["Moby_0001: moved"], []))
        before, after = moby_instances(self.data, GC), moby_instances(out, GC)
        self.assertEqual((before[0], before[2]), (after[0], after[2]))
        self.assertEqual(after[1]["matrix"][12:15], [7.0, 8.0, 9.0])
        self.assertAlmostEqual(after[1]["scale"], 3.0, places=5)
        self.assertEqual(after[1]["fields"], before[1]["fields"])

    def test_a_changed_value(self):
        def mission(index, node):
            return [line.replace("rc1_mission = 4", "rc1_mission = 9") for line in node]
        out, changes, _ = pack_level(self.data, scene(self.data, mission), GC)
        self.assertEqual(changes, ["Moby_0001: values"])
        self.assertEqual(moby_instances(out, GC)[1]["fields"]["mission"], 9)

    def test_adding_deleting_and_a_class_change_are_listed_not_packed(self):
        def reclass(index, node):
            if index == 0:
                node[0] = node[0].replace('"moby_500"', '"moby_7"')
                node.insert(0, '[ext_resource type="PackedScene" path="res://x/moby_7.tscn" id="moby_7"]')
            return node
        out, changes, problems = pack_level(self.data, scene(self.data, reclass, skip=(2,)), GC)
        self.assertEqual((out, changes), (self.data, []))
        self.assertEqual(len(problems), 2)
        self.assertIn("class 7", problems[0])
        self.assertIn("1 moby(s) deleted", problems[1])


if __name__ == "__main__":
    unittest.main()
