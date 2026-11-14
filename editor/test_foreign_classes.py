"""foreign_classes.py: a moby of another level's class, placed with its own logic."""

import struct
import tempfile
import unittest
from pathlib import Path

import foreign_classes
import gameplay as gp
from foreign_classes import Donor
from mobys import FIELDS, RECORD, moby_instances
from pack import pack_level
from test_pack import gameplay, scene

FOREIGN = ('[ext_resource type="PackedScene" path="res://library/rac1/ntsc/level_03/mobys/moby_909.glb" id="big_al"]\n\n'
           '[node name="BigAl_rac1_ntsc_L03" parent="Game/Mobys" instance=ExtResource("big_al")]\n'
           'transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4, 5, 6)\n'
           'metadata/rc1_foreign_level = 3\n')


class ForeignClasses(unittest.TestCase):
    def test_the_partial_switch_is_read_from_the_scene(self):
        text = ('[gd_scene format=3]\n\n[node name="Game" type="Node3D"]\n\n'
                '[node name="Mobys" type="Node3D" parent="Game"]\n\n'
                '[node name="A" parent="Game/Mobys" instance=ExtResource("a")]\n'
                'metadata/rc1_foreign_level = 8\nmetadata/rc1_foreign_partial = true\n')
        import unittest.mock as mock
        with mock.patch.object(foreign_classes, "scene_mobys", return_value=[
                {"class_id": 440, "meta": {"foreign_level": 8, "foreign_partial": True}},
                {"class_id": 347, "meta": {"foreign_level": 17}}]):
            self.assertEqual(foreign_classes.partial(text), {440})

    def test_a_place_in_its_pvars_moves_with_it(self):
        donor = foreign_classes.Donor(18, 1402, {}, pvar=struct.pack("<4f", 10.0, 20.0, 1.0, 2.0))
        move = foreign_classes.Move(0.0, 5.0, 6.0, -3.0)
        known = {(1402, 18): [(0, "z"), (4, "z")], (7, 1): [(0, "x"), (4, "y"), (8, "z")]}
        import positions
        saved = dict(positions._known)
        positions._known.clear()
        positions._known.update(known)
        try:
            self.assertEqual(struct.unpack("<4f", foreign_classes.moved_pvar(donor, move)), (7.0, 17.0, 1.0, 2.0))
            point = foreign_classes.Donor(1, 7, {}, pvar=struct.pack("<3f", 1.0, 0.0, 4.0))
            turned = foreign_classes.Move(3.141592653589793 / 2, 10.0, 0.0, 1.0)
            x, y, z = struct.unpack("<3f", foreign_classes.moved_pvar(point, turned))
            self.assertAlmostEqual(x, 10.0, places=5)
            self.assertAlmostEqual(y, 1.0, places=5)
            self.assertEqual(z, 5.0)
        finally:
            positions._known.clear()
            positions._known.update(saved)

    def test_the_scenes_imported_classes(self):
        self.assertEqual(foreign_classes.wanted(FOREIGN), ({909: 3}, []))
        self.assertEqual(foreign_classes.wanted(FOREIGN.replace("metadata/rc1_foreign_level = 3\n", "")), ({}, []))

    def test_a_class_from_two_levels_or_a_bad_level_is_reported(self):
        second = FOREIGN.split("\n\n", 1)[1].replace("BigAl_rac1_ntsc_L03", "BigAl_2").replace("= 3", "= 4")
        found, problems = foreign_classes.wanted(FOREIGN + "\n" + second)
        self.assertEqual(found, {909: 3})
        self.assertIn("BigAl_2: class 909 is already imported from level 03", problems[0])
        found, problems = foreign_classes.wanted(FOREIGN.replace("= 3", '= "Kerwan"'))
        self.assertEqual(found, {})
        self.assertIn("must be a level number", problems[0])

    def test_a_class_from_the_same_level_or_no_level_is_refused(self):
        foreign_classes.check(5, 3, 909)
        foreign_classes.check(3, 5, 909)         # a bigger program comes in too
        with self.assertRaises(ValueError):
            foreign_classes.check(3, 3, 909)
        with self.assertRaises(ValueError):
            foreign_classes.check(3, 19, 909)

    def test_the_moby_takes_after_its_levels_moby_with_its_own_pvars(self):
        data = gameplay([{"x": 1.0}, {"x": 2.0}])
        record = dict(zip(FIELDS, RECORD.unpack(gameplay([{"class_id": 909, "group": 4, "spawn_id": 77}])[0x110:0x110 + 0x78])))
        donor = Donor(3, 909, record, bytes(range(16)) + b"\x05\x00\x00\x00", [], [16], [(4, b"shared!")])
        out, changes, problems = pack_level(5, data, scene(data) + "\n" + FOREIGN, None, None, None, {909: [donor]})
        self.assertEqual(problems, [])
        self.assertIn("BigAl_rac1_ntsc_L03: added, class 909 from level 03", changes)
        added = moby_instances(out)[-1]
        self.assertEqual(added["class_id"], 909)
        # Its group there (4) becomes a group of its own here, of its copy.
        self.assertEqual(gp.groups(out)[added["fields"]["group"]], [added["index"]])
        self.assertNotEqual(added["fields"]["pvar_index"], -1)
        self.assertEqual(added["matrix"][12:15], [4.0, 5.0, 6.0])
        self.assertEqual(added["scale"], 1.0)
        blob, records = gp.shared(out)
        self.assertEqual(records, [(added["fields"]["pvar_index"], 4, 0)])
        self.assertEqual(blob[:7], b"shared!")

    def test_a_group_its_pvars_name_becomes_a_group_of_the_copies(self):
        data = gameplay([{"x": 1.0}])
        record = dict(zip(FIELDS, RECORD.unpack(gameplay([{"class_id": 909}])[0x110:0x110 + 0x78])))
        donor = Donor(3, 909, record, bytes(20), index=7, grouped=[(16, "<i", [7, 8])])
        out, _, problems = pack_level(5, data, scene(data) + "\n" + FOREIGN, None, None, None, {909: [donor]})
        self.assertEqual(problems, [])
        added = moby_instances(out)[-1]
        at, _ = gp.pvar_table(out, added["fields"]["pvar_index"] + 1)[added["fields"]["pvar_index"]]
        start, _ = gp.extent(out, gp.DATA)
        number, = struct.unpack_from("<i", out, start + at + 16)
        self.assertEqual(gp.groups(out)[number], [added["index"]])   # 8 did not come: left out

    def test_its_scale_is_the_nodes_over_its_class_scale(self):
        data = gameplay([{"x": 1.0}])
        record = dict(zip(FIELDS, RECORD.unpack(gameplay([{"class_id": 909}])[0x110:0x110 + 0x78])))
        node = FOREIGN.replace("Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1,", "Transform3D(0.5, 0, 0, 0, 0.5, 0, 0, 0, 0.5,")
        node += "metadata/rc1_foreign_class_scale = 0.25\n"
        out, _, problems = pack_level(5, data, scene(data) + "\n" + node, None, None, None, {909: [Donor(3, 909, record)]})
        self.assertEqual(problems, [])
        self.assertAlmostEqual(moby_instances(out)[-1]["scale"], 2.0, places=5)

    def test_the_mobys_its_links_name_come_with_it_moved(self):
        data = gameplay([{"x": 1.0}])
        record = dict(zip(FIELDS, RECORD.unpack(gameplay([{"class_id": 909, "x": 10.0, "y": 20.0}])[0x110:0x110 + 0x78])))
        lift = dict(zip(FIELDS, RECORD.unpack(gameplay([{"class_id": 685, "x": 12.0, "y": 20.0, "z": 1.0}])[0x110:0x110 + 0x78])))
        # Its block links the lift (moby 7 there) at +4.
        qwark = Donor(3, 909, record, bytes(4) + b"\xff" * 4 + bytes(8), [], [4], [], index=2, linked=[(4, 7)])
        group = [qwark, Donor(3, 685, lift, bytes(16), index=7)]
        out, changes, problems = pack_level(5, data, scene(data) + "\n" + FOREIGN, None, None, None, {909: group})
        self.assertEqual(problems, [])
        self.assertIn("BigAl_rac1_ntsc_L03: added, class 909 from level 03, with 1 moby(s) it names there", changes)
        mobys = moby_instances(out)
        self.assertEqual([m["class_id"] for m in mobys[-2:]], [909, 685])
        # Placed at (4, 5, 6) with no turn: the lift keeps its place beside it.
        self.assertEqual(mobys[-1]["matrix"][12:15], [6.0, 5.0, 7.0])
        pvars = gp.pvar_table(out, gp.pvar_count(out, [m["fields"]["pvar_index"] for m in mobys]))
        start, _ = gp.extent(out, gp.DATA)
        at, _ = pvars[mobys[-2]["fields"]["pvar_index"]]
        self.assertEqual(int.from_bytes(out[start + at + 4:start + at + 8], "little", signed=True), len(mobys) - 1)
        self.assertIn((mobys[-2]["fields"]["pvar_index"], 4), gp.fixups(out, gp.LINKS))

    def test_a_volume_turns_and_moves_with_its_moby(self):
        import math
        import struct
        move = foreign_classes.Move.between({"x": 1.0, "y": 0.0, "z": 0.0, "rz": 0.0},
                                            {"x": 10.0, "y": 10.0, "z": 2.0, "rz": math.pi / 2})
        self.assertEqual([round(v, 4) for v in move.point(1.0, 0.0, 0.0)], [10.0, 10.0, 2.0])
        self.assertEqual([round(v, 4) for v in move.point(2.0, 0.0, 0.0)], [10.0, 11.0, 2.0])
        # A box 2 by 1 by 1 at (2, 0, 0): its axes turn, its inverse with them.
        box = struct.pack("<32f", 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0.01,
                          0.5, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0)
        f = struct.unpack("<32f", foreign_classes.moved_shape(box, move))
        self.assertEqual([round(v, 4) for v in f[0:3] + f[4:7] + f[12:15]], [0, 2, 0, -1, 0, 0, 10, 11, 2])
        # The inverse takes the turned box's axes to the unit ones.
        axes = [f[0:3], f[4:7], f[8:11]]
        inverse = [f[16:19], f[20:23], f[24:27]]
        for i, axis in enumerate(axes):
            local = [sum(axis[k] * inverse[k][j] for k in range(3)) for j in range(3)]
            self.assertEqual([round(v, 4) for v in local], [1.0 if j == i else 0.0 for j in range(3)])
        self.assertAlmostEqual(f[30], math.pi / 2, places=5)

    def test_worlds_list_each_moby_and_what_it_brought(self):
        import struct
        empty = bytes(gp.SECTIONS * 4)
        record = {"class_id": 909, "x": 4.0, "y": 5.0, "z": 6.0, "rz": 0.0}
        lift = {"class_id": 685, "x": 6.0, "y": 5.0, "z": 7.0, "rz": 0.0}
        donor = Donor(3, 909, {"x": 1.0, "y": 0.0, "z": 0.0, "rz": 0.0})
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / "level_05"
            foreign_classes.write_worlds(out, [([(record, donor), (lift, Donor(3, 685, {}))], empty)])
            raw = (out / "worlds.bin").read_bytes()
            self.assertEqual(raw[:4], b"ORWD")
            version, worlds, bindings = struct.unpack_from("<3I", raw, 4)
            self.assertEqual((version, worlds, bindings), (1, 1, 2))
            first = 16 + 16 * worlds
            self.assertEqual(struct.unpack_from("<iI2f", raw, first), (909, 0, 4.0, 5.0))
            self.assertEqual(struct.unpack_from("<iI2f", raw, first + 16), (685, 0, 6.0, 5.0))
            foreign_classes.write_worlds(out, [])
            self.assertFalse((out / "worlds.bin").exists())

    def test_its_levels_tables_are_laid_out_as_the_loader_does(self):
        import struct
        # A file with one cuboid (+0x60), one path of two points (+0x70) and one camera (+0x8).
        head = [0] * gp.SECTIONS
        body = bytearray(gp.SECTIONS * 4)
        head[0x60 // 4] = len(body)
        body += struct.pack("<i12x", 1) + struct.pack("<32f", *([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 5, 6, 7, 0.01]
                                                            + [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0]))
        head[0x70 // 4] = len(body)
        body += struct.pack("<3i4x", 1, 0x20, 0x30) + struct.pack("<i12x", 0)
        body += struct.pack("<i12x", 2) + struct.pack("<8f", 1, 2, 3, -1, 4, 5, 6, -1)
        head[0x8 // 4] = len(body)
        body += struct.pack("<i12x", 1) + struct.pack("<i3f3fi", 17, 1, 1, 1, 0, 0, 0, -1)
        struct.pack_into(f"<{gp.SECTIONS}i", body, 0, *head)
        relocs, sets, data = foreign_classes.world(bytes(body), foreign_classes.Move(0.0, 10.0, 0.0, 0.0))
        by = {(g, i): (v, p) for g, i, v, p in sets}
        self.assertEqual(by[(foreign_classes.CUBOID_COUNT, 0)], (1, 0))
        cuboid, _ = by[(foreign_classes.CUBOIDS, 0)]
        self.assertEqual(struct.unpack_from("<3f", data, cuboid + 0x30), (15.0, 6.0, 7.0))
        self.assertEqual(by[(foreign_classes.PATH_COUNT, 0)], (1, 0))
        path, pointer = by[(foreign_classes.PATHS, 0)]
        self.assertEqual(pointer, 1)
        self.assertEqual(struct.unpack_from("<i", data, path)[0], 2)
        self.assertEqual(struct.unpack_from("<3f", data, path + 0x10), (11.0, 2.0, 3.0))
        self.assertEqual(struct.unpack_from("<3f", data, path + 0x20), (14.0, 5.0, 6.0))
        camera, _ = by[(foreign_classes.CAMERAS, 0)]
        self.assertEqual(struct.unpack_from("<3fi", data, camera), (11.0, 1.0, 1.0, 17))
        self.assertEqual(by[(foreign_classes.SPHERE_COUNT, 0)], (0, 0))
        self.assertEqual(relocs, [])

    def test_the_classes_a_class_makes_and_theirs(self):
        known = {(29, 13): {36, 1238}, (36, -1): {775}, (775, 13): {29}}
        import spawns
        self.assertEqual(spawns.made_by(29, 13, known), {36, 1238, 775})
        self.assertEqual(spawns.made_by(29, 12, known), set())
        self.assertTrue(spawns.table())    # the table the repository carries

    def test_writes_the_list_and_the_blobs_and_clears_them(self):
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / "level_05"
            done = foreign_classes.write(out, {909: (3, b"\x01" * 0x60)})
            self.assertEqual(done, ["class 909: imported from level 03"])
            self.assertEqual((out / "classes.tsv").read_text().splitlines()[1], "909\t3")
            self.assertEqual((out / "classes" / "909.bin").read_bytes(), b"\x01" * 0x60)
            foreign_classes.write(out, {})
            self.assertFalse((out / "classes.tsv").exists())
            self.assertFalse((out / "classes" / "909.bin").exists())


if __name__ == "__main__":
    unittest.main()
