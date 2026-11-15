"""The Assets dock end to end, in a real editor: test_editor_e2e.gd clicks, types and drags as a
person would in a headless editor on hand-built level projects (no disc), and checks what each
action did: search, filters, planets, the tree, each way of adding an asset, drags into the 3D
view and the Scene dock, undo and redo, old drops stood up, the foreign class switch, a scene
that is not a level, and the plugin switched off and on. One editor run does it all, in seconds:

    OPENRAC_GODOT=/path/to/godot python3 -m unittest test_editor_e2e -v

The editor's own errors fail it too: any ERROR line in its output that is not expected.
"""

import json
import os
import re
import shutil
import subprocess
import tempfile
import time
import unittest
from pathlib import Path

from test_library_dock import level_project

GODOT = os.environ.get("OPENRAC_GODOT")
HERE = Path(__file__).parent

# The level as the extractor writes it: Game turns the game's z-up to Godot's y-up, an extracted
# moby carries rc1_index, and StaticAssets holds one model an older editor dropped lying down
# (its up on the game's y) and one standing.
LEVEL = """[gd_scene format=3]

[ext_resource type="PackedScene" path="res://levels/level_05/mobys/moby_11.tscn" id="moby_11"]

[node name="Level" type="Node3D"]

[node name="Game" type="Node3D" parent="."]
transform = Transform3D(1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0)

[node name="Mobys" type="Node3D" parent="Game"]

[node name="Vendor_0000" parent="Game/Mobys" instance=ExtResource("moby_11")]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 2, 0)
metadata/rc1_index = 0

[node name="StaticAssets" type="Node3D" parent="Game"]

[node name="OldDrop" type="Node3D" parent="Game/StaticAssets"]
transform = Transform3D(2, 0, 0, 0, 0, -2, 0, 2, 0, 5, 6, 7)
metadata/rc1_asset = "rac1/ntsc/level_03/mobys/75"

[node name="Standing" type="Node3D" parent="Game/StaticAssets"]
transform = Transform3D(0.87758255, -0.47942555, 0, 0.47942555, 0.87758255, 0, 0, 0, 1, 1, 2, 3)
metadata/rc1_asset = "rac1/ntsc/level_03/mobys/75"
"""

# Errors the editor prints that are no fault of the dock: the fork's leaks at exit, and the
# broken assets' own (read on purpose).
EXPECTED_ERRORS: list[str] = ["were leaked at exit", "not a model"]

# Levels of another game that only make the library bigger, and their mobys each.
STRESS_LEVELS = 12
STRESS_MOBYS = 40


def projects(root: Path) -> Path:
    """Four levels of two games and two discs; the first is the one edited."""
    here = level_project(root, "rac1", "ntsc", 5, "Ratchet & Clank: Rilgar (NTSC-U)", {11: ("vendor", 0.25)}, [1000])
    level_project(root, "rac1", "ntsc", 3, "Ratchet & Clank: Kerwan (NTSC-U)",
                  {11: ("vendor", 0.25), 75: ("red_ship", 0.5), 1430: ("codebot_slot", 1.0)}, [682, 1000])
    level_project(root, "rac1", "pal", 3, "Ratchet & Clank: Kerwan (PAL)", {1201: ("turret", 0.5)}, [])
    level_project(root, "rac3", "ntsc", 1, "Ratchet & Clank: Up Your Arsenal: Veldin (NTSC-U)",
                  {10: ("something", 1.0)}, [])
    # Broken assets: a model whose texture is gone, and a file that is no model.
    broken = level_project(root, "rac1", "ntsc", 6, "Ratchet & Clank: Blarg Station (NTSC-U)",
                           {29: ("turret", 1.0), 30: ("gun", 1.0)}, [])
    (broken / "levels" / "level_06" / "textures" / "t.png").unlink()
    (broken / "levels" / "level_06" / "mobys" / "moby_30.glb").write_bytes(b"not a model")
    # Many levels of another game, for the list's speed.
    for number in range(STRESS_LEVELS):
        level_project(root, "rac4", "ntsc", number, f"Ratchet: Deadlocked: Arena {number} (NTSC-U)",
                      {100 + i: (f"thing_{i}", 1.0) for i in range(STRESS_MOBYS)}, [])
    (here / "levels" / "level_05" / "level_05.tscn").write_text(LEVEL)

    # The test's own plugin, enabled after the dock's.
    addon = here / "addons" / "openrac_e2e"
    addon.mkdir(parents=True)
    shutil.copy(HERE / "test_editor_e2e.gd", addon / "plugin.gd")
    (addon / "plugin.cfg").write_text('[plugin]\n\nname="OpenRAC e2e"\ndescription=""\nauthor=""\nversion="1"\n'
                                      'script="plugin.gd"\n')
    settings = here / "project.godot"
    text = settings.read_text()
    settings.write_text(re.sub(r'enabled=PackedStringArray\((.*)\)',
                               lambda m: f'enabled=PackedStringArray({m.group(1)}, "res://addons/openrac_e2e/plugin.cfg")', text))
    return here


@unittest.skipUnless(GODOT, "set OPENRAC_GODOT")
class AssetsDockEndToEnd(unittest.TestCase):
    def test_every_way_of_using_the_dock(self):
        with tempfile.TemporaryDirectory() as temp:
            here = projects(Path(temp) / "levels")
            out = Path(temp) / "result.json"
            began = time.monotonic()
            run = subprocess.run([GODOT, "--headless", "--editor", "--resolution", "1280x900", "--path", str(here),
                                  "res://levels/level_05/level_05.tscn", "--", str(out)],
                                 capture_output=True, text=True, timeout=180)
            log = run.stdout + run.stderr
            self.assertTrue(out.is_file(), log[-4000:])
            result = json.loads(out.read_text())
            print(f"\n  editor run {time.monotonic() - began:.1f} s, scenarios {result['total_ms']} ms: "
                  + ", ".join(f"{name} {ms}" for name, ms in result["times"].items()))

            for entry in result["checks"]:
                with self.subTest(scenario=entry["scenario"], check=entry["name"]):
                    self.assertTrue(entry["ok"], entry["detail"])

            # Every scenario ran (one that stops on a script error leaves the ones after it out).
            self.assertEqual(result["stopped_in"], "", "a scenario stopped: see the ERROR lines")
            self.assertEqual(len(result["times"]), 29, result["times"])
            errors = [line for line in log.splitlines()
                      if re.search(r"\b(ERROR|SCRIPT ERROR)\b", line)
                      and not any(expected in line for expected in EXPECTED_ERRORS)]
            self.assertEqual(errors, [], "\n".join(errors[:20]))


if __name__ == "__main__":
    unittest.main()
