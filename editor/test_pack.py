"""pack.pack_level on a made-up gameplay file and scene."""

import math
import struct
import unittest

import gameplay as gp
from godot import transform
from mobys import FIELDS, POINTER, RECORD, SIZE, USUAL, moby_instances
from pack import pack_level, placement, scene_mobys, virtual_range


def gameplay(records: list[dict]) -> bytes:
    start = 0x100
    out = bytearray(start + 0x10 + len(records) * SIZE)
    struct.pack_into("<I", out, POINTER, start)
    struct.pack_into("<i", out, start, len(records))
    for i, fields in enumerate(records):
        record = {"size": SIZE, **USUAL, "spawn_id": i, "group": -1, "pvar_index": -1, "class_id": 500,
                  "scale": 1.0, "x": 0.0, "y": 0.0, "z": 0.0, "rx": 0.0, "ry": 0.0, "rz": 0.0,
                  "r": 10, "g": 20, "b": 30, **fields}
        RECORD.pack_into(out, start + 0x10 + i * SIZE, *(record[f] for f in FIELDS))
    return bytes(out)


def scene(data: bytes, edit=None) -> str:
    """The moby nodes as the extractor writes them; EDIT changes the lines of one node."""
    lines = ['[ext_resource type="PackedScene" path="res://levels/level_00/mobys/moby_500.tscn" id="moby_500"]',
             '[ext_resource type="PackedScene" path="res://levels/level_00/mobys/moby_7.tscn" id="moby_7"]', ""]
    for p in moby_instances(data):
        node = [f'[node name="Moby_{p["index"]:04}" parent="Game/Mobys" instance=ExtResource("moby_{p["class_id"]}")]',
                "rotation_order = 0", f"transform = {transform(p['matrix'])}", f"metadata/rc1_index = {p['index']}"]
        node += [f"metadata/rc1_{k} = {v}" for k, v in p["fields"].items()]
        if edit:
            node = edit(p["index"], node)
        lines += node + [""]
    return "\n".join(lines)


class PackLevel(unittest.TestCase):
    def setUp(self):
        self.data = gameplay([{"x": 1.5, "y": -2.25, "z": 3.0, "rz": 0.7, "scale": 2.0},
                              {"x": 10.0, "rx": 0.3, "ry": -0.4, "rz": 1.1, "draw_distance": 128},
                              {"x": -5.0, "y": 0.1}])

    def test_untouched_scene_changes_nothing(self):
        out, changes, problems = pack_level(0, self.data, scene(self.data))
        self.assertEqual((out, changes, problems), (self.data, [], []))

    def test_godots_own_saving_changes_nothing(self):
        """Godot adds a type and a unique_id to the node, writes rc1_class on each instance and
        rounds floats its own way."""
        def as_godot(index, node):
            node[0] = node[0].replace(' parent=', ' type="Node3D" parent=').replace(" instance=", " unique_id=77 instance=")
            return node + ["metadata/rc1_class = 500", 'metadata/rc1_class_name = "BoltCrate"',
                           'metadata/rc1_source = "games/rac1/decomp/src/example.c"',
                           'metadata/rc1_update = "UpdateCrate"']
        out, changes, problems = pack_level(0, self.data, scene(self.data, as_godot))
        self.assertEqual((out, changes, problems), (self.data, [], []))

    def test_a_moved_moby_changes_only_its_record(self):
        def move(index, node):
            if index == 1:
                node[2] = "transform = Transform3D(0, -3, 0, 3, 0, 0, 0, 0, 3, 7, 8, 9)"
            return node
        out, changes, problems = pack_level(0, self.data, scene(self.data, move))
        self.assertEqual(changes, ["Moby_0001: moved"])
        self.assertEqual(problems, [])
        before, after = moby_instances(self.data), moby_instances(out)
        self.assertEqual((before[0], before[2]), (after[0], after[2]))
        self.assertEqual(after[1]["matrix"][12:15], [7.0, 8.0, 9.0])
        self.assertAlmostEqual(after[1]["scale"], 3.0, places=5)
        self.assertAlmostEqual(after[1]["rotation"][2], math.pi / 2, places=5)
        self.assertEqual(after[1]["fields"], before[1]["fields"])

    def test_values_and_class(self):
        def edit(index, node):
            if index == 2:
                node[0] = node[0].replace("moby_500", "moby_7")
                node = [line.replace("rc1_spawn_id = 2", "rc1_spawn_id = 99") for line in node]
                node.append("metadata/rc1_draw_distance = 200")
            return node
        out, changes, problems = pack_level(0, self.data, scene(self.data, edit))
        self.assertEqual(changes, ["Moby_0002: class 500 -> 7, values"])
        after = moby_instances(out)[2]
        self.assertEqual((after["class_id"], after["fields"]["spawn_id"], after["fields"]["draw_distance"]), (7, 99, 200))



def level() -> bytes:
    """A gameplay file with what belongs to its mobys: three of them (the second with pvars, in
    group 1 with the third), seven spare runtime mobys, two pvar blocks whose first word is a
    moby's place in the table (a link) and whose second is a pointer into the block, and a
    shared record for the second block."""
    out = bytearray(gameplay([{"x": 1.0}, {"x": 2.0, "pvar_index": 1, "group": 1}, {"x": 3.0, "group": 1}]))
    struct.pack_into("<i", out, 0x100 + 4, 7)

    def section(pointer: int, data: bytes) -> None:
        out.extend(bytes(-len(out) % 16))
        struct.pack_into("<i", out, pointer, len(out))
        out.extend(data)

    section(gp.GROUPS, gp.write_groups([None, [1, 2]]))
    section(gp.TABLE, struct.pack("<4i", 0, 16, 16, 16))
    section(gp.DATA, struct.pack("<8i", 2, 4, 0, 0, 0, 8, 0x55, 0))
    section(gp.LINKS, gp.write_fixups([(0, 0), (1, 0)]))
    section(gp.POINTERS, gp.write_fixups([(1, 4)]))
    section(gp.SHARED, gp.write_shared(b"\x07" * 16, [(1, 8, 4)]))
    out.extend(bytes(-len(out) % 16))
    return bytes(out)


class AddAndDelete(unittest.TestCase):
    def setUp(self):
        self.data = level()

    def pvar(self, data: bytes, index: int) -> tuple:
        at, size = gp.pvar_table(data, index + 1)[index]
        return struct.unpack_from(f"<{size // 4}i", data, gp.offset(data, gp.DATA) + at)

    def reclass(self, data: bytes, index: int, class_id: int) -> bytes:
        out = bytearray(data)
        at = 0x100 + 0x10 + index * SIZE
        record = dict(zip(FIELDS, RECORD.unpack_from(out, at)))
        record["class_id"] = class_id
        RECORD.pack_into(out, at, *(record[f] for f in FIELDS))
        return bytes(out)

    def test_a_class_change_takes_a_copy_of_the_new_classes_pvars(self):
        # Moby 1 is of class 7 with a pvar block; moby 0 becomes class 7 and gets its own copy.
        data = self.reclass(self.data, 1, 7)
        def edit(index, node):
            return [node[0].replace("moby_500", "moby_7")] + node[1:] if index == 0 else node
        out, changes, problems = pack_level(0, data, scene(data, edit))
        self.assertEqual(problems, [])
        after = moby_instances(out)
        moved = after[0]["fields"]["pvar_index"]
        self.assertEqual(after[0]["class_id"], 7)
        self.assertNotIn(moved, (-1, 1))
        self.assertEqual(self.pvar(out, moved)[1:], self.pvar(out, 1)[1:])

    def test_a_class_change_with_old_pvars_and_no_donor_is_refused(self):
        def edit(index, node):
            return [node[0].replace("moby_500", "moby_7")] + node[1:] if index == 1 else node
        out, changes, problems = pack_level(0, self.data, scene(self.data, edit))
        self.assertTrue(any("class is not changed" in p for p in problems), problems)
        self.assertEqual(moby_instances(out)[1]["class_id"], 500)

    def test_a_copy_is_a_new_moby_with_a_spawn_number_pvars_and_a_place_in_its_group(self):
        def edit(index, node):
            if index != 1:
                return node
            copy = [node[0].replace("Moby_0001", "Moby_0003")] + node[1:]
            copy[2] = "transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 9, 8, 7)"
            return node + [""] + copy
        out, changes, problems = pack_level(0, self.data, scene(self.data, edit))
        self.assertEqual(problems, [])
        self.assertEqual(changes, ["Moby_0003: added, after moby 1"])
        mobys = moby_instances(out)
        self.assertEqual(len(mobys), 4)
        self.assertEqual(gp.records(out)[:3], gp.records(self.data))            # the table's own: as they were
        self.assertEqual(struct.unpack_from("<i", out, gp.offset(out, gp.MOBYS) + 4)[0], 7)   # the spare mobys
        new = mobys[3]["fields"]
        self.assertEqual(mobys[3]["matrix"][12:15], [9.0, 8.0, 7.0])
        self.assertEqual((new["spawn_id"], new["pvar_index"], new["group"]), (3, 2, 1))
        self.assertEqual(gp.groups(out), [None, [1, 2, 3]])
        self.assertEqual(self.pvar(out, 2), self.pvar(out, 1))                  # a block of its own, the same
        self.assertNotEqual(gp.pvar_table(out, 3)[2][0], gp.pvar_table(out, 3)[1][0])
        self.assertEqual(gp.pvar_table(out, 3)[2][0] % 16, 0)
        self.assertEqual(gp.fixups(out, gp.LINKS), [(0, 0), (1, 0), (2, 0)])
        self.assertEqual(gp.fixups(out, gp.POINTERS), [(1, 4), (2, 4)])
        self.assertEqual(gp.shared(out), (b"\x07" * 16, [(1, 8, 4), (2, 8, 4)]))

    def test_a_class_scene_dropped_in_takes_after_the_levels_first_moby_of_its_class(self):
        text = scene(self.data) + ('\n[node name="Crate" parent="Game/Mobys" instance=ExtResource("moby_500")]\n'
                                   "transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4, 5, 6)\n")
        out, changes, problems = pack_level(0, self.data, text)
        self.assertEqual((changes, problems), (["Crate: added, after moby 0"], []))
        new = moby_instances(out)[3]
        self.assertEqual((new["class_id"], new["matrix"][12:15], new["fields"]["spawn_id"]), (500, [4.0, 5.0, 6.0], 3))

    def test_a_skin_or_a_behaviour_is_not_a_value(self):
        text = scene(self.data, lambda i, node: node + ['metadata/rc1_behaviour = "spin"',
                                                        "metadata/rc1_behaviour_speed = 90",
                                                        'metadata/rc1_skin = "res://library/skins/a/model.glb"']
                     if i == 1 else node)
        text += ('\n[node name="Crate" parent="Game/Mobys" instance=ExtResource("moby_500")]\n'
                 "transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4, 5, 6)\n"
                 'metadata/rc1_behaviour = "bob"\n')
        out, changes, problems = pack_level(0, self.data, text)
        self.assertEqual((changes, problems), (["Crate: added, after moby 0"], []))

    def test_a_deleted_moby_closes_the_table_and_what_names_mobys_follows(self):
        out, changes, problems = pack_level(0, self.data, scene(self.data, lambda i, node: [] if i == 1 else node))
        self.assertEqual((changes, problems), (["BoltCrate_0001: deleted"], []))
        self.assertEqual(gp.records(out), [gp.records(self.data)[0], gp.records(self.data)[2]])
        self.assertEqual(gp.groups(out), [None, [1]])                           # the third moby is now the second
        self.assertEqual(self.pvar(out, 0)[0], 1)                               # block 0 linked to it: follows
        self.assertEqual(self.pvar(out, 1)[0], 0)                               # block 1's is the first: stays
        self.assertEqual(len(out), len(self.data))                              # nothing grew: all in place

    def test_the_packed_table_names_its_nodes(self):
        # moby_states.py after_pack: each packed moby's node, in the packed table's order.
        names = []
        text = scene(self.data, lambda i, node: [] if i == 1 else node) + (
            '\n[node name="Crate" parent="Game/Mobys" instance=ExtResource("moby_500")]\n')
        pack_level(0, self.data, text, None, names)
        self.assertEqual(names[1:], ["Moby_0002", "Crate"])
        self.assertEqual(len(names), 3)
        unchanged = []
        pack_level(0, self.data, scene(self.data), None, unchanged)
        self.assertEqual(len(unchanged), 3)
        self.assertTrue(unchanged[2].endswith("_0002"))

    def test_a_link_to_a_deleted_moby_becomes_none(self):
        out, _, _ = pack_level(0, self.data, scene(self.data, lambda i, node: [] if i == 2 else node))
        self.assertEqual(self.pvar(out, 0)[0], -1)
        self.assertEqual(gp.groups(out), [None, [1]])


class Placement(unittest.TestCase):
    def test_angles_come_back(self):
        from mobys import matrix
        from pack import scene_transform
        for rotation in ((0.3, -0.4, 1.1), (0.0, 0.0, 0.0), (-2.0, 1.2, 3.0), (0.5, math.pi / 2, 0.0)):
            m = matrix((1.0, 2.0, 3.0), rotation, 1.75)
            position, angles, scale = placement(scene_transform(m))
            again = matrix(position, angles, scale)
            for a, b in zip(m, again):
                self.assertAlmostEqual(a, b, places=6)

    def test_uneven_scale_is_refused(self):
        from formats import FormatError
        with self.assertRaises(FormatError):
            placement((2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0))

    def test_scene_reader(self):
        nodes = scene_mobys('[node name="X" parent="Game/Mobys"]\nmetadata/rc1_index = 4\nmetadata/rc1_colour = [1, 2, 3]\n')
        self.assertEqual(nodes[0]["meta"], {"index": 4, "colour": [1, 2, 3]})
        self.assertEqual(nodes[0]["transform"][:3], (1.0, 0.0, 0.0))


class Names(unittest.TestCase):
    def test_a_node_is_named_after_its_class(self):
        from mobys import node_name
        names = {11: "vendor", 8: "blarg_heavy_interceptor"}
        self.assertEqual(node_name(11, 12, names), "Vendor_0012")
        self.assertEqual(node_name(8, 40, names), "BlargHeavyInterceptor_0040")
        self.assertEqual(node_name(809, 1, names), "Class809_0001")

    def test_any_name_packs(self):
        """The packer goes by rc1_index, not by the node's name."""
        data = gameplay([{"x": 1.0}])
        text = scene(data).replace("Moby_0000", "Vendor_0000")
        self.assertEqual(pack_level(0, data, text), (data, [], []))


class VirtualRanges(unittest.TestCase):
    def test_reserves_adjacent_non_overlapping_ranges(self):
        cursor = [0x10000000]
        first = virtual_range(cursor, 7)
        second = virtual_range(cursor, 11)
        self.assertEqual((first, second, cursor[0]), (0x10000000, 0x10000007, 0x10000012))

    def test_rejects_sector_address_space_overflow(self):
        from formats import FormatError
        with self.assertRaisesRegex(FormatError, "32-bit sector address space"):
            virtual_range([0xffffffff], 1)


class PackDisc(unittest.TestCase):
    """pack.pack on either disc: the copy its game loads is the one replaced."""

    def test_both_native_ports_pack_the_60hz_copy_on_their_own_disc(self):
        import json
        import tempfile
        from pathlib import Path
        import disc
        from formats import wad
        from pack import pack
        from wadpack import compress
        data = gameplay([{"x": 1.0}, {"x": 2.0}])
        stored = compress(data)
        alternate = compress(gameplay([{"x": 99.0}, {"x": 2.0}]))
        sectors = -(-max(len(stored), len(alternate)) // disc.SECTOR)
        survey = {"levels": [{"id": 0, "ranges": {"gameplay_ntsc": {"lba": 100, "sectors": sectors},
                                                  "gameplay_pal": {"lba": 200, "sectors": sectors}}}]}

        def move(index, node):
            if index == 1:
                node[2] = "transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5, 6, 7)"
            return node
        for version, lba in ((disc.NTSC, 100), (disc.PAL, 100)):
            class Disc:
                pass
            image = Disc()
            image.version = version
            reads = []
            def read(first, count):
                reads.append(first)
                payload = {100: stored, 200: alternate}[first]
                return payload + bytes(count * disc.SECTOR - len(payload))
            image.sectors = read
            with tempfile.TemporaryDirectory() as tmp:
                project, out = Path(tmp) / "levels", Path(tmp) / "packed"
                (project / "level_00/levels/level_00").mkdir(parents=True)
                metadata = project / "level_00/levels/level_00/level.json"
                if version.key == "pal":
                    from formats import FormatError
                    (project / "level_00/levels/level_00/level_00.tscn").write_text(scene(data, move))
                    with self.assertRaises(FormatError) as refused:
                        pack(image, survey, project, out)
                    self.assertEqual(refused.exception.code, "project-outdated-gameplay")
                    self.assertFalse(out.exists())
                metadata.write_text(json.dumps({"gameplay_source": "gameplay_ntsc"}))
                (project / "level_00/levels/level_00/level_00.tscn").write_text(scene(data, move))
                self.assertEqual(pack(image, survey, project, out), (1, 1))
                name = f"level_00/gameplay_{version.key}.wad"
                self.assertEqual((out / "patches.tsv").read_text().splitlines()[1], f"{lba}\t{sectors}\t{name}")
                self.assertEqual(moby_instances(wad((out / name).read_bytes()))[1]["matrix"][12:15], [5.0, 6.0, 7.0])
                self.assertEqual(moby_instances(wad((out / name).read_bytes()))[0], moby_instances(data)[0])
                self.assertEqual(reads, [100])
                self.assertEqual(json.loads((out / "packed.json").read_text())["version"], version.key)
                # An unedited scene must not import the other copy's differences.
                (project / "level_00/levels/level_00/level_00.tscn").write_text(scene(data))
                self.assertEqual(pack(image, survey, project, Path(tmp) / "untouched"), (0, 0))
                self.assertEqual(len((Path(tmp) / "untouched/patches.tsv").read_text().splitlines()), 1)

    def test_gameplay_larger_than_its_disc_slot_uses_virtual_sectors_and_a_header_patch(self):
        import tempfile
        from pathlib import Path
        from unittest.mock import patch
        import disc
        import pack as pack_module

        original, edited = b"original gameplay", b"edited gameplay"
        stored = b"compressed original" + bytes(disc.SECTOR - len(b"compressed original"))
        expanded_stream = b"WAD" + bytes(2046)
        header = bytes(disc.LEVEL_HEADER_SIZE)

        class Disc:
            version = disc.NTSC

            def sectors(self, first, count):
                testcase.assertEqual((first, count), (100, 1))
                return stored

            def read(self, offset, size):
                testcase.assertEqual((offset, size), (50 * disc.SECTOR, disc.LEVEL_HEADER_SIZE))
                return header

        survey = {"levels": [{"id": 0, "header_lba": 50,
                              "ranges": {"gameplay_ntsc": {"lba": 100, "sectors": 1},
                                         "gameplay_pal": {"lba": 200, "sectors": 1}}}]}
        with tempfile.TemporaryDirectory() as temporary:
            testcase = self
            project, out = Path(temporary) / "levels", Path(temporary) / "packed"
            level_dir = project / "level_00/levels/level_00"
            level_dir.mkdir(parents=True)
            (level_dir / "level_00.tscn").write_text("[gd_scene format=3]\n")
            decode = lambda data: original if data == stored else edited
            with patch.object(pack_module, "decoded", side_effect=decode), \
                    patch.object(pack_module, "pack_level", return_value=(edited, ["Moby_0000: edited"], [])), \
                    patch.object(pack_module, "compress", return_value=expanded_stream), \
                    patch.object(pack_module, "pack_collision", return_value=([], [], False)), \
                    patch("scenery.edited", return_value=False), \
                    patch("static_assets.placements", return_value=[]), \
                    patch("static_assets.write_packed", return_value=None):
                self.assertEqual(pack_module.pack(Disc(), survey, project, out), (1, 1))
            first, count, filename, level, slot, original_first, original_count = \
                (out / "virtual.tsv").read_text().splitlines()[1].split("\t")
            self.assertEqual((int(first), int(count), filename, int(level), int(slot),
                              int(original_first), int(original_count)),
                             (0x10000000, 2, "level_00/gameplay_ntsc.wad", 0, 1, 100, 1))
            patched_header = (out / "level_00/header_ntsc.bin").read_bytes()
            self.assertEqual(struct.unpack_from("<II", patched_header, 8 + int(slot) * 8),
                             (int(first), int(count)))
            self.assertEqual((out / "patches.tsv").read_text().splitlines()[1],
                             "50\t5\tlevel_00/header_ntsc.bin")
            self.assertEqual((out / filename).stat().st_size, int(count) * disc.SECTOR)


if __name__ == "__main__":
    unittest.main()


class PackCutscenes(unittest.TestCase):
    """The level's cutscenes (OpenRAC Editor writes them beside the scene) go into the pack."""

    def test_cutscenes_and_their_files_are_copied_and_broken_ones_left_out(self):
        import json
        import tempfile
        from pathlib import Path
        from pack import pack_cutscenes
        with tempfile.TemporaryDirectory() as tmp:
            scene_dir, out = Path(tmp) / "level_03", Path(tmp) / "packed" / "level_03"
            cutscenes = scene_dir / "cutscenes"
            (cutscenes / "audio").mkdir(parents=True)
            (cutscenes / "hello.cutscene").write_text(json.dumps({"format": "openrac-cutscene", "version": 1, "length": 60}))
            (cutscenes / "broken.cutscene").write_text("{not json")
            (cutscenes / "other.cutscene").write_text(json.dumps({"format": "something-else"}))
            (cutscenes / "list.cutscene").write_text("[]")
            (cutscenes / "audio" / "line_en.wav").write_bytes(b"RIFF")
            (cutscenes / "audio" / "line_en.wav.import").write_text("[remap]")
            (cutscenes / ".hidden").write_text("x")
            changes, problems = pack_cutscenes(scene_dir, out)
            self.assertEqual(changes, ["cutscene hello"])
            self.assertEqual(len(problems), 3, problems)
            self.assertTrue(all(p.startswith("cutscene ") for p in problems))
            copied = sorted(str(p.relative_to(out)) for p in out.rglob("*") if p.is_file())
            self.assertEqual(copied, ["cutscenes/audio/line_en.wav", "cutscenes/hello.cutscene"])

    def test_checks_before_packing(self):
        import json
        import tempfile
        from pathlib import Path
        from pack import check_cutscene, pack_cutscenes
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            (source / "audio").mkdir()
            (source / "audio" / "hi_en.wav").write_bytes(b"RIFF")
            (source / "layers").mkdir()
            (source / "layers" / "wave.bin").write_bytes(bytes(16 * 2 * 3))
            good = {"format": "openrac-cutscene", "level": 3, "length": 120,
                    "trigger": {"kind": "talk", "spawn_id": 7},
                    "actors": [{"id": "vendor", "source": {"spawn_id": 7}},
                               {"id": "guard", "source": {"class": 500},
                                "layers": [{"name": "wave", "joints": [52, 53], "frames": "layers/wave.bin"}]}],
                    "camera": {"keys": [{"t": 0, "eye": {"actor": "vendor"}}]},
                    "sounds": [{"t": 0, "file": {"en": "audio/hi_en.wav"}, "actor": "vendor"}],
                    "events": [{"t": 5, "do": "moby_hide", "spawn_id": 7}]}
            asked = []
            self.assertEqual(check_cutscene(good, source, 3, {7}, lambda: asked.append(1) or {500}), ([], []))
            self.assertEqual(asked, [1])  # the classes only when one is spawned
            bad = json.loads(json.dumps(good))
            bad["level"] = 4
            bad["length"] = 0
            bad["actors"][1]["source"] = {"class": 501}
            bad["actors"].append({"id": "vendor", "source": {"figure": "hero"}})
            errors, _ = check_cutscene(bad, source, 3, {7}, {500})
            self.assertEqual(len(errors), 4, errors)
            odd = json.loads(json.dumps(good))
            odd["trigger"]["spawn_id"] = 8
            odd["actors"][0]["source"] = {"spawn_id": 9}
            odd["actors"][1]["layers"][0]["joints"] = [52, 53, 54, 55]
            odd["actors"][1]["ride"] = [{"t": 0, "actor": "ship"}]
            odd["camera"]["keys"][0]["eye"]["actor"] = "nobody"
            odd["sounds"][0]["file"] = {"en": "audio/hi_en.wav", "fr": "audio/hi_fr.wav"}
            odd["sounds"].append({"t": 1, "file": "../outside.wav"})
            odd["events"][0]["spawn_id"] = 99
            errors, warnings = check_cutscene(odd, source, 3, {7}, {500})
            self.assertEqual(errors, [])
            self.assertEqual(len(warnings), 8, warnings)
            # Not checked against what is not known.
            self.assertEqual(check_cutscene(odd | {"trigger": {}}, source, None, None, None)[0], [])
            figures = {"format": "openrac-cutscene", "length": 10,
                       "actors": [{"id": str(i), "source": {"figure": "hero"}} for i in range(9)]}
            self.assertEqual(len(check_cutscene(figures, source)[0]), 1)
            # Written by hand, any shape: checked, never a crash.
            shapes = {"format": "openrac-cutscene", "length": 10, "actors": 3, "sounds": {"t": 1},
                      "events": None, "camera": [], "trigger": "volume"}
            self.assertEqual(check_cutscene(shapes, source, 3, {7}, {500}), ([], []))
            odd_shapes = {"format": "openrac-cutscene", "length": 10,
                          "actors": [{"id": [1], "source": {"class": [500], "spawn_id": {}}, "layers": 4,
                                      "ride": [{"actor": {}}], "look_at": True}],
                          "sounds": [{"t": 1, "actor": [2], "file": "a\0b"}],
                          "events": [{"do": "moby_kill", "spawn_id": [7]}]}
            errors, warnings = check_cutscene(odd_shapes, source, 3, {7}, {500})
            self.assertEqual(len(errors), 1, errors)   # the class [500] is no class the level loads
            # In the pack: an error leaves it out, a warning keeps it.
            scene_dir, out = Path(tmp) / "level_03", Path(tmp) / "packed" / "level_03"
            (scene_dir / "cutscenes").mkdir(parents=True)
            (scene_dir / "cutscenes" / "fine.cutscene").write_text(json.dumps(good | {"actors": [], "camera": {}, "sounds": []}))
            (scene_dir / "cutscenes" / "wrong.cutscene").write_text(json.dumps(good | {"level": 5}))
            changes, problems = pack_cutscenes(scene_dir, out, 3, {7}, {500})
            self.assertEqual(changes, ["cutscene fine"])
            self.assertTrue(any(p.startswith("cutscene wrong: made for level 5") and p.endswith("left out")
                                for p in problems), problems)
            self.assertFalse((out / "cutscenes" / "wrong.cutscene").exists())

    def test_no_cutscenes_writes_nothing(self):
        import tempfile
        from pathlib import Path
        from pack import pack_cutscenes
        with tempfile.TemporaryDirectory() as tmp:
            scene_dir, out = Path(tmp) / "level_03", Path(tmp) / "packed" / "level_03"
            scene_dir.mkdir()
            self.assertEqual(pack_cutscenes(scene_dir, out), ([], []))
            (scene_dir / "cutscenes").mkdir()
            (scene_dir / "cutscenes" / "audio.wav").write_bytes(b"RIFF")
            # Files with no cutscene to play them are not carried.
            self.assertEqual(pack_cutscenes(scene_dir, out), ([], []))
            self.assertFalse(out.exists())

    def test_a_link_out_of_the_folder_is_not_followed(self):
        import json
        import os
        import tempfile
        from pathlib import Path
        from pack import pack_cutscenes
        with tempfile.TemporaryDirectory() as tmp:
            scene_dir, out = Path(tmp) / "level_03", Path(tmp) / "packed" / "level_03"
            (scene_dir / "cutscenes").mkdir(parents=True)
            (scene_dir / "cutscenes" / "a.cutscene").write_text(json.dumps({"format": "openrac-cutscene", "length": 60}))
            secret = Path(tmp) / "secret.txt"
            secret.write_text("private")
            os.symlink(secret, scene_dir / "cutscenes" / "secret.txt")
            pack_cutscenes(scene_dir, out)
            self.assertFalse((out / "cutscenes" / "secret.txt").exists())

    def test_a_level_with_only_cutscenes_is_packed(self):
        import json
        import tempfile
        from pathlib import Path
        import disc
        from pack import pack
        from wadpack import compress
        data = gameplay([{"x": 1.0}, {"x": 2.0}])
        stored = compress(data)
        sectors = -(-len(stored) // disc.SECTOR)
        survey = {"levels": [{"id": 0, "ranges": {"gameplay_ntsc": {"lba": 100, "sectors": sectors},
                                                  "gameplay_pal": {"lba": 200, "sectors": sectors}}}]}

        class Disc:
            version = disc.NTSC

            def sectors(self, first, count):
                return stored + bytes(count * disc.SECTOR - len(stored))
        with tempfile.TemporaryDirectory() as tmp:
            project, out = Path(tmp) / "levels", Path(tmp) / "packed"
            level = project / "level_00/levels/level_00"
            (level / "cutscenes").mkdir(parents=True)
            # The cutscene's nodes in the scene, as the editor saves them, are not mobys.
            nodes = ('\n[node name="Cutscenes" type="Node3D" parent="Game"]\n'
                     '\n[node name="Intro" type="Cutscene" parent="Game/Cutscenes"]\n'
                     'transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5, 6, 7)\n'
                     '\n[node name="Vendor" type="CutsceneActor" parent="Game/Cutscenes/Intro"]\n'
                     'spawn_id = 149\n'
                     '\n[node name="Timeline" type="AnimationPlayer" parent="Game/Cutscenes/Intro"]\n')
            (level / "level_00.tscn").write_text(scene(data) + nodes)
            (level / "cutscenes" / "intro.cutscene").write_text(json.dumps({"format": "openrac-cutscene", "length": 60}))
            self.assertEqual(pack(Disc(), survey, project, out), (1, 1))
            self.assertTrue((out / "level_00/cutscenes/intro.cutscene").is_file())
            self.assertIn("cutscene intro", (out / "changes.txt").read_text())
            # Nothing else changed: no sector of the disc is replaced.
            self.assertEqual(len((out / "patches.tsv").read_text().splitlines()), 1)
