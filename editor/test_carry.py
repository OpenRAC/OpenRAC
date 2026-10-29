"""carry.py: a player's edits from an outdated level project, carried onto a fresh one."""

import json
import tempfile
import unittest
from pathlib import Path

import carry
import extract
from mobys import moby_instances
from pack import pack_level
from test_pack import gameplay, scene

# The fresh project's gameplay copy: the same mobys, one value the game's own (a 60 Hz one).
OLD = gameplay([{"x": 1.0}, {"x": 2.0, "draw_distance": 64}, {"x": 3.0}])
NEW = gameplay([{"x": 1.0}, {"x": 2.0, "draw_distance": 96}, {"x": 3.0}])


def edited(edit) -> str:
    return "[gd_scene format=3]\n\n" + scene(OLD, edit)


def fresh() -> str:
    return "[gd_scene format=3]\n\n" + scene(NEW)


class CarryLevel(unittest.TestCase):
    def test_an_untouched_project_carries_nothing_and_the_fresh_scene_stays_as_written(self):
        text, report = carry.carry_level(edited(None), OLD, fresh(), NEW)
        self.assertEqual(text, fresh())
        self.assertEqual((report["carried"], report["not_carried"]), ([], []))

    def test_a_moved_moby_and_a_changed_value_are_carried_and_the_rest_is_the_fresh_ones(self):
        def edit(index, node):
            if index == 0:
                node[2] = "transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5, 6, 7)"
            if index == 2:
                node = [line.replace("rc1_spawn_id = 2", "rc1_spawn_id = 42") for line in node]
            return node
        text, report = carry.carry_level(edited(edit), OLD, fresh(), NEW)
        self.assertEqual(report["carried"], ["Moby_0000: moved", "Moby_0002: values"])
        packed, changes, problems = pack_level(0, NEW, text)
        self.assertEqual(problems, [])
        after = moby_instances(packed)
        self.assertEqual(after[0]["matrix"][12:15], [5.0, 6.0, 7.0])
        self.assertEqual(after[2]["fields"]["spawn_id"], 42)
        # Moby 1 was not edited: it keeps the fresh copy's value, not the old one's.
        self.assertEqual(after[1]["fields"]["draw_distance"], 96)

    def test_deleted_and_added_mobys_are_carried(self):
        old = edited(None)
        lines = old.split("\n\n")
        copy = next(b for b in lines if 'name="Moby_0001"' in b).replace('name="Moby_0001"', 'name="Moby_0001_2"')
        old = "\n\n".join([b for b in lines if 'name="Moby_0002"' not in b] + [copy, ""])
        text, report = carry.carry_level(old, OLD, fresh(), NEW)
        self.assertEqual(sorted(report["carried"]), ["Moby_0001_2: added", "Moby_0002: deleted"])
        packed, changes, problems = pack_level(0, NEW, text)
        self.assertEqual(problems, [])
        self.assertEqual(len(moby_instances(packed)), 3)
        self.assertIn("BoltCrate_0002: deleted", changes)

    def test_a_class_change_points_the_node_at_the_other_class_scene(self):
        def edit(index, node):
            if index == 1:
                node[0] = node[0].replace("moby_500", "moby_7")
            return node
        text, report = carry.carry_level(edited(edit), OLD, fresh(), NEW)
        self.assertEqual(report["carried"], ["Moby_0001: class"])
        self.assertEqual(moby_instances(pack_level(0, NEW, text)[0])[1]["class_id"], 7)

    def test_a_value_an_older_editor_did_not_write_is_left_as_the_fresh_one(self):
        def edit(index, node):
            return [line for line in node if "rc1_draw_distance" not in line] if index == 1 else node
        text, report = carry.carry_level(edited(edit), OLD, fresh(), NEW)
        self.assertEqual(report["carried"], [])
        self.assertEqual(text, fresh())

    def test_tables_in_another_order_match_by_class_and_spawn_number(self):
        new = gameplay([{"x": 3.0, "spawn_id": 2}, {"x": 1.0, "spawn_id": 0, "class_id": 7}, {"x": 2.0, "spawn_id": 1}])
        old = gameplay([{"x": 1.0, "spawn_id": 0, "class_id": 7}, {"x": 2.0, "spawn_id": 1}, {"x": 3.0, "spawn_id": 2}])

        def edit(index, node):
            if index == 2:
                node[2] = "transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 9, 9, 9)"
            return node
        text, report = carry.carry_level("[gd_scene format=3]\n\n" + scene(old, edit), old,
                                         "[gd_scene format=3]\n\n" + scene(new), new)
        self.assertEqual(report["carried"], ["Moby_0002: moved"])
        self.assertEqual(moby_instances(pack_level(0, new, text)[0])[0]["matrix"][12:15], [9.0, 9.0, 9.0])

    def test_a_scene_that_lost_most_of_its_mobys_carries_nothing(self):
        old = "[gd_scene format=3]\n\n" + scene(OLD).split("\n\n")[0] + "\n"
        with self.assertRaises(carry.FormatError) as caught:
            carry.carry_level(old, OLD, fresh(), NEW)
        self.assertEqual(caught.exception.code, "carry-failed")


class CarryProject(unittest.TestCase):
    def test_the_report_is_written_beside_the_fresh_scene(self):
        def edit(index, node):
            if index == 0:
                node[2] = "transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5, 6, 7)"
            return node
        with tempfile.TemporaryDirectory() as tmp:
            old, new = Path(tmp) / "old" / "level_03", Path(tmp) / "new" / "level_03"
            for folder, text in ((old, edited(edit)), (new, fresh())):
                folder.mkdir(parents=True)
                (folder / "level_03.tscn").write_text(text)
            report = carry.carry_project(old.parent, old, new, OLD, NEW, Path(tmp) / "backup")
            self.assertEqual(report["carried"], ["Moby_0000: moved"])
            self.assertEqual(json.loads((new / carry.REPORT).read_text())["level"], 3)
            self.assertIn("5, 6, 7", (new / "level_03.tscn").read_text())


class UpdateGodot(unittest.TestCase):
    def test_a_replaced_level_is_handed_to_carry_with_its_backup(self):
        from test_extract import fake_godot
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp).resolve() / "levels" / "pal"
            extract.export_godot(dest, [0], True, True, fake_godot(extract.GODOT_SCHEMA))
            seen = []

            def carrying(level, old, new, backup):
                seen.append((level, old.relative_to(backup).as_posix(), new.relative_to(dest).as_posix()))
                return {"level": level, "carried": ["Moby_0000: moved"], "not_carried": ["Camera_0000: moved"],
                        "mobys": 1}
            said = extract.export_godot(dest, [0], True, True,
                                        fake_godot(extract.GODOT_SCHEMA, gameplay_source="gameplay_ntsc"),
                                        gameplay_source="gameplay_ntsc", carry=carrying)
            self.assertEqual(seen, [(0, "level_00/levels/level_00", "level_00/levels/level_00")])
            self.assertIn("1 edit(s) carried over, 1 to redo by hand", said)

    def test_a_failing_carry_keeps_the_fresh_level_and_says_so(self):
        from test_extract import fake_godot
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp).resolve() / "levels" / "pal"
            extract.export_godot(dest, [0], True, True, fake_godot(extract.GODOT_SCHEMA - 1))

            def broken(*_):
                raise KeyError("boom")
            report = extract.update_godot(dest, [0], True, fake_godot(extract.GODOT_SCHEMA), extract.GODOT_SCHEMA,
                                          carry=broken)
            self.assertTrue(report["carried"][0]["failed"])
            self.assertEqual(extract.level_schema(dest / "level_00" / "levels" / "level_00"), extract.GODOT_SCHEMA)


class Repair(unittest.TestCase):
    def test_a_level_with_files_missing_is_backed_up_and_written_again(self):
        from test_extract import fake_godot
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp).resolve() / "levels" / "pal"
            export = fake_godot(extract.GODOT_SCHEMA)
            extract.export_godot(dest, [0, 1], True, True, export)
            (dest / "level_01" / "levels" / "level_01" / "level_01.tscn").unlink()
            report = extract.update_godot(dest, [0, 1], True, export, extract.GODOT_SCHEMA)
            self.assertEqual((report["kept"], report["replaced"]), ([0], [1]))
            self.assertTrue((dest / "level_01" / "levels" / "level_01" / "level_01.tscn").is_file())


if __name__ == "__main__":
    unittest.main()
