"""The copy of OpenRAC an installed launcher carries: its actions and its files."""
import hashlib
import json
import re
import shutil
import subprocess
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import bundle_standalone as bundle


def game_format(game: str) -> str:
    """The format of a game's export that moves on alone (extractor/port.py GAME_FORMATS)."""
    text = (bundle.ROOT / "extractor" / "port.py").read_text(encoding="utf-8")
    table = re.search(r'^GAME_FORMATS = (\{.*\})$', text, re.MULTILINE).group(1)
    return re.search(rf'"{game}": (\d+)', table).group(1)


def going_commando_format() -> str:
    """The format of Going Commando's export, which moves on alone (extractor/port.py GAME_FORMATS)."""
    return game_format("rac2")


class LevelsStamp(unittest.TestCase):
    """The check's stamp names the export's format in every place that reads or writes it: an old
    name leaves Play waiting for a stamp the game never writes, the check run again and again."""

    def test_every_stamp_names_the_export_format(self):
        expected = bundle.levels_format()
        header = (bundle.ROOT / "port" / "scene" / "level.h").read_text(encoding="utf-8")
        self.assertEqual(int(re.search(r"kManifestFormat = (\d+);", header).group(1)), expected)
        actions = (bundle.ROOT / "launcher" / "actions.json").read_text(encoding="utf-8")
        stamps = set(re.findall(r"verified-(\d+)\.json", actions))
        self.assertEqual(stamps, {str(expected)})
        # The levels are ready by a stamp of their own, named after the format too, so a bump
        # reads them again before the check runs.
        # Going Commando's export moved on alone, so its bump reads only its levels again.
        self.assertEqual(set(re.findall(r"levels-(\d+)\.json", actions)),
                         {str(expected), going_commando_format(), game_format("rac3")})
        self.assertEqual(set(re.findall(r"rac3/(?:ntsc|pal)/levels-(\d+)\.json", actions)),
                         {game_format("rac3")})
        self.assertEqual(set(re.findall(r"rac2/ntsc/levels-(\d+)\.json", actions)), {going_commando_format()})
        self.assertEqual(bundle.LEVELS_STAMP, f"levels-{expected}.json")
        extract = (bundle.ROOT / "extractor" / "extract.py").read_text(encoding="utf-8")
        self.assertIn('f"levels-{levels_format(game)}.json"', extract)
        self.assertNotIn("level_99/manifest.json", actions)


class PlayerActions(unittest.TestCase):
    def setUp(self):
        self.actions = bundle.player_actions()
        self.pal = {a["id"]: a for a in self.actions["versions"]["rac1/pal"]}
        self.ntsc = {a["id"]: a for a in self.actions["versions"]["rac1/ntsc"]}

    def test_an_apk_plays_only_the_games_it_carries(self):
        def platforms(actions, key):
            return next(a for a in actions["versions"][key] if a["id"] == "play").get("platforms", [])
        without = bundle.player_actions(android=True, android_games={"rac1-pal", "rac1-ntsc"})
        self.assertNotIn("android", platforms(without, "rac2/ntsc"))
        self.assertIn("linux", platforms(without, "rac2/ntsc"))
        with_rac2 = bundle.player_actions(android=True, android_games={"rac1-pal", "rac1-ntsc", "rac2-ntsc"})
        self.assertIn("android", platforms(with_rac2, "rac2/ntsc"))
        self.assertIn("android", platforms(bundle.player_actions(android=True), "rac2/ntsc"))

    def test_only_what_a_player_does(self):
        self.assertEqual(self.actions["repository"], [])
        for key, actions in self.actions["versions"].items():
            self.assertLessEqual({a["id"] for a in actions}, set(bundle.KEEP), key)
        # Playing, and modding: every player gets the Mods page, not only a checkout's.
        self.assertEqual(set(self.pal), set(bundle.KEEP))
        self.assertEqual(set(self.ntsc), set(bundle.KEEP))

    def test_the_later_games_levels_are_read_for_packs_in_the_data_folder(self):
        # Going Commando, Up Your Arsenal and Deadlocked: their levels are read into the data
        # folder for texture and model packs. Going Commando's are packed back there too, the
        # early access games' not.
        for key, stamp in (("rac2/ntsc", going_commando_format()), ("rac3/ntsc", game_format("rac3")),
                           ("rac4/ntsc", 4)):
            with self.subTest(key):
                actions = {a["id"]: a for a in self.actions["versions"][key]}
                # The level editor's steps come with OpenRAC Editor, not in the copy.
                self.assertFalse([i for i in actions if i.startswith("editor-")])
                levels = actions["port-levels"]
                self.assertEqual(levels["args"][-3:], ["{data}/port/ntsc", "--anywhere", "--replace"])
                self.assertEqual(levels["artifact"], f"{{data}}/port/ntsc/levels-{stamp}.json")
                self.assertEqual([n["path"] for n in actions["assets-export"]["needs"]], [levels["artifact"]])

    def test_audio_exports_to_the_data_folder_on_each_disc(self):
        for actions in (self.pal, self.ntsc):
            self.assertEqual(actions["audio-export"]["args"][3], "{data}/exports/audio-level_{level}-{option}")
            self.assertEqual(actions["audio-export-all"]["args"][3], "{data}/exports/audio-{option}")
            self.assertIn("--timestamp", actions["audio-export"]["args"])
            self.assertIn("--timestamp", actions["audio-export-all"]["args"])
            self.assertEqual(actions["audio-import"]["args"][-1], "{data}/overrides")

    def test_mods_work_in_the_data_folder(self):
        stack = self.pal["overrides-stack"]["args"]
        self.assertEqual(stack[stack.index("--packed") + 1], "{data}/packed/pal")
        self.assertEqual(stack[stack.index("--levels") + 1], "{data}/port/pal")
        exported = self.pal["assets-export"]
        self.assertEqual(exported["args"][2], "{data}/port/pal")
        self.assertEqual([need["path"] for need in exported["needs"]], ["{data}/port/pal/levels-4.json"])
        edited = self.pal["play-edited"]
        self.assertEqual(edited["args"][edited["args"].index("--levels") + 1], "{data}/port/pal")
        self.assertEqual([need["path"] for need in edited["needs"]],
                         [self.pal["disc-files"]["makes"], self.pal["port-levels-check"]["makes"]])
        self.assertEqual(edited["cwd"], "{data}")
        # NTSC-U's folders are its own.
        stack = self.ntsc["overrides-stack"]["args"]
        self.assertEqual(stack[stack.index("--packed") + 1], "{data}/packed/ntsc")

    def test_interface_runs_read_the_data_folders_levels_and_the_players_run(self):
        for name, actions in (("pal", self.pal), ("ntsc", self.ntsc)):
            run = actions["interface-run"]["args"]
            self.assertEqual(run[run.index("--levels") + 1], f"{{data}}/port/{name}")
            self.assertEqual(run[run.index("--out") + 1], f"{{data}}/interface_runs/rac1-{name}.json")
            export = actions["interface-export"]["args"]
            self.assertFalse(any("port/game/" in arg for arg in export))
            self.assertIn(f"{{data}}/interface_runs/rac1-{name}.json", export)

    def test_no_mod_action_names_the_checkout(self):
        for action in [*self.pal.values(), *self.ntsc.values()]:
            text = json.dumps({k: action.get(k) for k in ("args", "artifact", "needs", "makes")})
            self.assertNotIn("assets/", text.replace("{data}", ""), action["id"])

    def test_levels_go_to_the_data_folder(self):
        levels = self.pal["port-levels"]
        self.assertEqual(levels["args"][-3:], ["{data}/port/pal", "--anywhere", "--replace"])
        self.assertEqual(levels["artifact"], "{data}/port/pal/levels-4.json")

    def test_play_reads_them_there(self):
        play = self.pal["play"]
        self.assertEqual(play["args"][play["args"].index("--levels") + 1], "{data}/port/pal")
        # Play waits for the check's stamp, and the check for the export.
        check = self.pal["port-levels-check"]
        self.assertEqual(check["makes"], "{data}/port/pal/verified-4.json")
        self.assertEqual(check["args"][check["args"].index("--levels") + 1], "{data}/port/pal")
        self.assertEqual([need["path"] for need in check["needs"]], [self.pal["port-levels"]["artifact"]])
        # And for the disc's files, which the game reads in place of the image.
        files = self.pal["disc-files"]
        self.assertEqual(files["makes"], "{data}/iso_data/rac1/sectors.tsv")
        self.assertEqual(files["args"][:2], ["tools/extractor.py", "{disc}"])
        self.assertEqual(files["args"][-2:], ["--proj-path", "{data}"])
        self.assertEqual(files["cwd"], ".")
        self.assertEqual([need["path"] for need in play["needs"]], [files["makes"], check["makes"]])
        self.assertNotIn("python", play.get("requires", []))

    def test_ci_smoke_checks_use_only_packaged_native_bake_entry_points(self):
        workflow = (bundle.ROOT/'.github/workflows/launcher.yml').read_text()
        self.assertNotIn('$COPY/port/enhanced/tools/baker/',workflow)
        self.assertIn('"$baker" --print-water-profile',workflow)
        self.assertIn('"$baker" --print-interior-profile',workflow)

    def test_the_probe_bake_reads_and_writes_the_data_folder(self):
        for version, actions in (("pal", self.pal), ("ntsc", self.ntsc)):
            bake = actions["probe-bake"]
            args = bake["args"]
            self.assertEqual(bake["program"], "{artifact}")
            self.assertIn("--bake-all", args)
            self.assertNotIn("--source-decoder", args)
            self.assertNotIn("python", bake.get("requires", []))
            self.assertNotIn("--baker", args)
            self.assertEqual(args[args.index("--levels-dir") + 1], f"{{data}}/port/{version}")
            self.assertEqual(args[args.index("--output-dir") + 1], f"{{data}}/probes/{version}")
            self.assertIn("--update", args)
            self.assertEqual([need["path"] for need in bake["needs"]], [actions["port-levels-check"]["makes"]])
            self.assertTrue(bake["artifact"].endswith("openrac-enhanced-baker.exe"))

    def test_ntsc_levels_have_a_folder_of_their_own(self):
        # <data> is the game's, shared by PAL and NTSC-U: each version's levels are <data>/port/<version>.
        levels, check = self.ntsc["port-levels"], self.ntsc["port-levels-check"]
        self.assertEqual(levels["args"][-3:], ["{data}/port/ntsc", "--anywhere", "--replace"])
        self.assertEqual(levels["artifact"], "{data}/port/ntsc/levels-4.json")
        at = check["args"].index("--levels")
        self.assertEqual(check["args"][at + 1], "{data}/port/ntsc")
        self.assertEqual(check["makes"], "{data}/port/ntsc/verified-4.json")
        self.assertEqual([need["path"] for need in check["needs"]], [levels["artifact"]])
        self.assertTrue(check["artifact"].endswith("openrac-rac1-ntsc.exe"))

    def test_going_commando_plays_for_players(self):
        # Its Play is carried as it is (a player's), started in the data
        # folder, reading the data folder's levels. It has no level check yet, so it waits for the
        # disc set up there and for the export's own stamp: a format bump reads the levels again
        # before it starts.
        actions = {a["id"]: a for a in self.actions["versions"]["rac2/ntsc"]}
        play = actions["play"]
        self.assertNotIn("developer", play)
        self.assertNotIn("warning", play)
        bake = actions["probe-bake"]
        self.assertNotIn("developer", bake)
        self.assertEqual(bake["args"][bake["args"].index("--levels-dir") + 1], "{data}/port/ntsc")
        self.assertEqual([need["path"] for need in bake["needs"]], [actions["port-levels"]["artifact"]])
        self.assertEqual(play["args"], ["--data", "{data}", "--window", "--levels", "{data}/port/ntsc"])
        self.assertEqual(play["cwd"], "{data}")
        self.assertEqual([n["path"] for n in play["needs"]],
                         ["{data}/iso_data/rac2/sectors.tsv", actions["port-levels"]["artifact"]])
        self.assertEqual(actions["port-levels"]["artifact"], f"{{data}}/port/ntsc/levels-{going_commando_format()}.json")
        self.assertTrue(play["artifact"].endswith("openrac-rac2-ntsc.exe"))
        self.assertNotIn("todo", play)
        # Like Ratchet & Clank, the copy carries the program: nothing is built on the player's computer,
        # and nothing but the disc is asked for.
        self.assertNotIn("port-build", actions)
        self.assertNotIn("port-part", actions)
        self.assertEqual(play["artifact"], "port/build/release/openrac-rac2-ntsc.exe")
        for game in ("openrac-rac2-ntsc", "openrac-rac2-pal", "openrac-rac2-ntsc2"):
            self.assertIn(game, bundle.GAMES)

    def test_android_plays_going_commando_with_no_part(self):
        actions = {a["id"]: a for a in bundle.player_actions(android=True)["versions"]["rac2/ntsc"]}
        self.assertNotIn("port-build", actions)
        self.assertNotIn("port-part", actions)
        self.assertNotIn("--asm-part", actions["play"]["args"])
        self.assertEqual(actions["play"]["artifact"], "port/build/release/openrac-rac2-ntsc.exe")

    def test_folders_and_arguments(self):
        self.assertEqual(bundle.levels_folder("rac1/pal"), "{data}/port/pal")
        self.assertEqual(bundle.levels_folder("rac1/ntsc"), "{data}/port/ntsc")
        self.assertEqual(bundle.levels_folder("rac9/ntsc-j"), "{data}/port/ntsc-j")
        args = ["--data", "{data}", "--levels", "{root}/assets/port/x", "--window"]
        self.assertEqual(bundle.point_at(args, "{data}/port/x"), ["--data", "{data}", "--levels", "{data}/port/x", "--window"])
        self.assertEqual(bundle.point_at(["--data", "{data}"], "{data}/port/x"), ["--data", "{data}"])

    def test_every_action_starts_in_a_folder_the_copy_has(self):
        # 0.4.0 started the game in games/rac1, which only a checkout has: Play failed with
        # "the directory name is invalid".
        self.assertEqual(self.pal["play"]["cwd"], "{data}")
        self.assertIn(self.pal["port-levels"]["cwd"], (".", "{root}"))

    def test_nothing_writes_in_the_copy(self):
        for action in [*self.pal.values(), *self.ntsc.values()]:
            for arg in action.get("args", []):
                self.assertFalse(arg.startswith(("assets/", "build/", "{root}/assets", "{root}/build")), arg)


class Bundle(unittest.TestCase):
    def test_the_extractor_is_copied_without_the_editor(self):
        # OpenRAC Editor's tools import the extractor's modules (extractor_api.json lists them).
        api = json.loads((bundle.ROOT / "extractor/extractor_api.json").read_text())
        for name in api["modules"]:
            self.assertTrue(bundle.extractor_file(Path(f"{name}.py")), name)
        self.assertTrue(bundle.extractor_file(Path("extractor_api.json")))
        self.assertFalse(bundle.extractor_file(Path("test_port.py")))
        # The guide the Audio tab's "How it works" opens.
        self.assertTrue(bundle.extractor_file(Path("AUDIO_MODDING.md")))

    def test_a_copy_without_game_or_python(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "openrac"
            import sys
            argv, sys.argv = sys.argv, ["bundle_standalone.py", str(out)]
            try:
                self.assertEqual(bundle.main(), 0)
            finally:
                sys.argv = argv
            self.assertIn("version: " + json.loads((bundle.ROOT/"launcher/src-tauri/tauri.conf.json").read_text())["version"],
                          (out/"STANDALONE").read_text())
            for rel in ("STANDALONE", "tools/openrac.py", "tools/extractor.py", "extractor/extract.py",
                        "extractor/moby_classes.tsv", "games/rac1/game.json", "progress/summary.json",
                        "launcher/actions.json",
                        "port/enhanced/lighting/profiles/rac1/moby-seeds.json",
                        "port/enhanced/lighting/profiles/rac1/object-lights.json",
                        "port/enhanced/lighting/profiles/rac1-ntsc/material-overrides.json",
                        "port/enhanced/lighting/profiles/rac1-ntsc/level_01/probe-bake.json"):
                self.assertTrue((out / rel).is_file(), rel)
            aridia = Path(bundle.PROBE_PROFILES) / "rac1-ntsc/level_02"
            for name in ("emission-textures.json", "emission-masks/red-ring-mask.png"):
                rel = aridia / name
                self.assertEqual((out / rel).read_bytes(), (bundle.ROOT / rel).read_bytes())
            self.assertFalse(list(out.rglob("test_*.py")))
            for name in ('bake_all.py', 'bake_level.py', 'seed_regions.py', 'native_coordinator.py'):
                self.assertFalse((out / "port/enhanced/tools/baker" / name).exists())
            self.assertFalse((out / "port/enhanced/tools/water").exists())
            self.assertFalse((out / "port/enhanced/tools/baker").exists())
            self.assertFalse((out / "games/rac1/decomp/include/moby_pvars.h").exists())
            layouts = json.loads((out / "extractor/pvar_layouts.json").read_text())
            self.assertIn(["moveSpeed", 0xB0, "f32", 0], layouts["classes"]["726"])
            # Resolve layouts from the installed copy, in a process that cannot accidentally
            # reuse the source checkout's cached module or headers.
            script = ("import sys\n"
                      "sys.path.insert(0, sys.argv[1])\n"
                      "import pvar_layouts\n"
                      "assert not (pvar_layouts.DECOMPILATION / 'include/moby_pvars.h').exists()\n"
                      "assert ('moveSpeed', 0xb0, 'f32', 0) in pvar_layouts.class_layouts()[726]\n")
            subprocess.run([sys.executable, "-I", "-c", script, str(out / "extractor")], cwd=tmp, check=True)
            json.loads((out / "launcher/actions.json").read_text())
            # The copy's tools run from it: their helpers are carried, and find the copy's root.
            self.assertTrue((out / "tools/openrac_common/__init__.py").is_file())
            # An interface run's tool and the routes it plays.
            done = subprocess.run([sys.executable, str(out / "port/tools/interface_runs/run.py"), "--help"], cwd=tmp,
                                  capture_output=True, text=True)
            self.assertNotIn("Traceback", done.stderr)
            self.assertTrue((out / "port/tools/regress/routes.json").is_file())
            for tool in ("openrac.py", "extractor.py"):
                done = subprocess.run([sys.executable, str(out / "tools" / tool), "--help"], cwd=tmp,
                                      capture_output=True, text=True)
                self.assertNotIn("Traceback", done.stderr, tool)
            root = subprocess.run([sys.executable, "-c", "from openrac_common import paths; print(paths.repo_root())"],
                                  cwd=out / "tools", capture_output=True, text=True, check=True).stdout.strip()
            self.assertEqual(Path(root), out.resolve())
            # The carried exporter resolves the carried profile without the checkout or its cwd.
            exported = Path(tmp) / "exported-level"
            exported.mkdir()
            catalog_path = out / "port/enhanced/lighting/profiles/rac1-ntsc/material-overrides.json"
            self.assertEqual(catalog_path.read_bytes(),
                             (bundle.ROOT / catalog_path.relative_to(out)).read_bytes())
            # A synthetic material proves the isolated exporter consumes the carried catalog.
            texture = b"bundled emission fixture"
            (exported / "textures").mkdir()
            (exported / "textures/lamp.png").write_bytes(texture)
            catalog = json.loads(catalog_path.read_text())
            definition = {"runtime_intensity": 3, "bake_intensity": 5}
            catalog["overrides"].append({
                "id": "fixture-lamp",
                "match": {"texture_file_sha256": hashlib.sha256(texture).hexdigest()},
                "set": {"emission": definition},
            })
            catalog_path.write_text(json.dumps(catalog))
            script = ("import sys\n"
                      "from pathlib import Path\n"
                      "from types import SimpleNamespace\n"
                      "sys.path.insert(0, sys.argv[1])\n"
                      "from enhanced.profiles import write_bake_profile\n"
                      "write_bake_profile(SimpleNamespace(game='rac1', version='ntsc', id=1), Path(sys.argv[2]))\n")
            subprocess.run([sys.executable, "-I", "-c", script,
                            str(out / "extractor"), str(exported)], cwd=tmp, check=True)
            profile = "port/enhanced/lighting/profiles/rac1-ntsc/level_01/probe-bake.json"
            self.assertEqual((exported / "probe-bake.json").read_bytes(), (bundle.ROOT / profile).read_bytes())
            lights = "port/enhanced/lighting/profiles/rac1/object-lights.json"
            self.assertEqual((exported / "object-lights.json").read_bytes(),
                             (out / lights).read_bytes())
            profiles = out / "port/enhanced/lighting/profiles"
            for layer, source in (("shared", "rac1"), ("version", "rac1-ntsc"),
                                  ("level", "rac1-ntsc/level_01")):
                exported_catalog = exported / "materials" / layer / "material-overrides.json"
                self.assertEqual(exported_catalog.read_bytes(),
                                 (profiles / source / "material-overrides.json").read_bytes())
            copied = json.loads((exported / "probe-emission.json").read_text())
            defaults = copied.pop("_catalog_defaults")
            self.assertEqual(copied, defaults)
            self.assertEqual(copied["textures/lamp.png"], definition)
            self.assertEqual(copied["textures/moby_0172.png"],
                             {"runtime_intensity": 1, "bake_intensity": 4, "contrast": 1.5})
            self.assertEqual(copied["static_mobys"], [1042])
            self.assertFalse((out / "port/enhanced/tools/baker/moby_seed_whitelist.json").exists())
            water_profile = "port/enhanced/water/profiles/rac1-ntsc/level_01/water-boundaries.json"
            self.assertEqual((out / water_profile).read_bytes(), (bundle.ROOT / water_profile).read_bytes())
            self.assertEqual((exported / "water-boundaries.json").read_bytes(), (bundle.ROOT / water_profile).read_bytes())
            ocean_profile = "port/enhanced/water/profiles/rac1-ntsc/level_11/ocean-profile.json"
            ocean_export = Path(tmp) / "exported-ocean"
            ocean_export.mkdir()
            subprocess.run([sys.executable, "-I", "-c", script.replace("id=1)", "id=11)"),
                            str(out / "extractor"), str(ocean_export)], cwd=tmp, check=True)
            self.assertEqual((ocean_export / "ocean-profile.json").read_bytes(),
                             (bundle.ROOT / ocean_profile).read_bytes())

    def test_unified_baker_is_packaged_without_companion_executables(self):
        for suffix in ("", ".exe"):
            with self.subTest(suffix=suffix), tempfile.TemporaryDirectory() as tmp:
                build, dest = Path(tmp)/"build", Path(tmp)/"dest"
                build.mkdir()
                dest.mkdir()
                baker = build/("openrac-enhanced-baker"+suffix)
                baker.write_bytes(b"enhanced baker")
                baker.chmod(0o755)
                (build/"openrac-enhanced-baker.pdb").write_bytes(b"debug")
                stamp = build/"openrac-build.json"
                stamp.write_text('{"version":"0.10.1","commit":"built-commit","games":[]}', encoding="utf-8")
                carried = bundle.copy_games(build, dest)
                self.assertEqual((dest/stamp.name).read_bytes(), stamp.read_bytes())
                self.assertEqual(carried, [baker.name])
                self.assertEqual((dest/baker.name).read_bytes(), baker.read_bytes())
                self.assertFalse((dest/"openrac-enhanced-baker.pdb").exists())
                if sys.platform != "win32":
                    self.assertTrue((dest/baker.name).stat().st_mode & 0o111)

    def test_games_as_built(self):
        # On macOS a game is its bundle, carried whole; the build folder's link to it is not.
        with tempfile.TemporaryDirectory() as tmp:
            build, dest = Path(tmp) / "build", Path(tmp) / "dest"
            app = "apps/openrac-rac1-pal/Ratchet & Clank.app"
            (build / app / "Contents/MacOS").mkdir(parents=True)
            (build / app / "Contents/MacOS/Ratchet & Clank").write_bytes(b"pal")
            (build / app / "Contents/Info.plist").write_text("<plist/>")
            (build / "openrac-rac1-pal").symlink_to(f"{app}/Contents/MacOS/Ratchet & Clank")
            # Elsewhere a game is one file (and a .pdb is left behind).
            (build / "openrac-rac1-ntsc.exe").write_bytes(b"ntsc")
            (build / "openrac-rac1-ntsc.pdb").write_bytes(b"")
            (build / "openrac-rac2-ntsc.exe").write_bytes(b"rac2")
            # A states program is one file on every system, macOS too, and carried beside the games.
            (build / "openrac-rac1-pal-states").write_bytes(b"states")
            (build / "openrac-rac1-ntsc-states.exe").write_bytes(b"states")
            dest.mkdir()
            self.assertEqual(bundle.copy_games(build, dest),
                             ["apps/openrac-rac1-pal", "openrac-rac1-ntsc.exe", "openrac-rac2-ntsc.exe",
                              "openrac-rac1-pal-states", "openrac-rac1-ntsc-states.exe"])
            self.assertEqual((dest / app / "Contents/MacOS/Ratchet & Clank").read_bytes(), b"pal")
            self.assertTrue((dest / app / "Contents/Info.plist").is_file())
            self.assertEqual(sorted(p.name for p in dest.iterdir()),
                             ["apps", "openrac-rac1-ntsc-states.exe", "openrac-rac1-ntsc.exe", "openrac-rac1-pal-states",
                              "openrac-rac2-ntsc.exe"])

    def test_windows_games_carry_the_cpp_runtime(self):
        with tempfile.TemporaryDirectory() as tmp:
            system, dest = Path(tmp) / "system", Path(tmp) / "dest"
            system.mkdir()
            dest.mkdir()
            for name in bundle.WINDOWS_RUNTIME:
                (system / name).write_bytes(name.encode())
            self.assertEqual(bundle.copy_windows_runtime(system, dest), list(bundle.WINDOWS_RUNTIME))
            for name in bundle.WINDOWS_RUNTIME:
                self.assertEqual((dest / name).read_bytes(), name.encode())
            # One that is missing stops the installer here, not the game on a player's computer.
            (system / "msvcp140_2.dll").unlink()
            with self.assertRaisesRegex(SystemExit, "msvcp140_2.dll"):
                bundle.copy_windows_runtime(system, dest)

    @unittest.skipUnless(sys.platform.startswith("linux") and shutil.which("cc")
                         and (shutil.which("llvm-strip") or shutil.which("strip")), "needs a Linux compiler and strip")
    def test_linux_programs_go_without_their_debug_info(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, program = Path(tmp) / "main.c", Path(tmp) / "openrac-rac1-pal"
            source.write_text("int main(void) { return 0; }\n")
            subprocess.run(["cc", "-g", "-o", str(program), str(source)], check=True)
            self.assertIn(b".debug_info", program.read_bytes())
            self.assertTrue(bundle.strip_debug(program))
            self.assertNotIn(b".debug_info", program.read_bytes())
            self.assertIn(b"main", program.read_bytes())  # the symbols stay
            subprocess.run([str(program)], check=True)
            # Not an ELF program (a Windows .exe): left as it is.
            exe = Path(tmp) / "openrac-rac1-pal.exe"
            exe.write_bytes(b"MZ")
            self.assertFalse(bundle.strip_debug(exe))
            self.assertEqual(exe.read_bytes(), b"MZ")

    def test_the_python_copy_keeps_what_the_tools_use(self):
        with tempfile.TemporaryDirectory() as tmp:
            home, out = Path(tmp) / "python", Path(tmp) / "out"
            for rel in ("bin/python3.12", "bin/pip3", "bin/python3-config", "lib/libpython3.12.so.1.0",
                        "lib/libtcl9.0.so", "lib/tcl9.0/init.tcl", "lib/python3.12/os.py",
                        "lib/python3.12/lib-dynload/_ssl.so", "lib/python3.12/lib-dynload/_tkinter.so",
                        "lib/python3.12/site-packages/pip/__init__.py", "lib/python3.12/test/a.py",
                        "lib/python3.12/unittest/__init__.py"):
                (home / rel).parent.mkdir(parents=True, exist_ok=True)
                (home / rel).write_text(rel)
            os.symlink("python3.12", home / "bin/python3")
            os.symlink("python3.12", home / "bin/python")
            bundle.copy_python(home, out)
            kept = sorted(p.relative_to(out).as_posix() for p in out.rglob("*") if p.is_file())
            self.assertEqual(kept, ["bin/python3", "lib/python3.12/lib-dynload/_ssl.so",
                                    "lib/python3.12/os.py", "lib/python3.12/unittest/__init__.py"])
            # The program itself, not a link to a file the copy no longer has.
            self.assertFalse((out / "bin/python3").is_symlink())
            self.assertEqual((out / "bin/python3").read_text(), "bin/python3.12")

    def test_a_windows_python_copy(self):
        with tempfile.TemporaryDirectory() as tmp:
            home, out = Path(tmp) / "python", Path(tmp) / "out"
            for rel in ("python.exe", "pythonw.exe", "python312.dll", "DLLs/_ssl.pyd", "DLLs/tcl86t.dll",
                        "DLLs/_tkinter.pyd", "Lib/os.py", "Lib/site-packages/pip/__init__.py", "tcl/init.tcl",
                        "libs/python312.lib", "include/Python.h", "Scripts/pip.exe"):
                (home / rel).parent.mkdir(parents=True, exist_ok=True)
                (home / rel).write_text(rel)
            bundle.copy_python(home, out)
            kept = sorted(p.relative_to(out).as_posix() for p in out.rglob("*") if p.is_file())
            self.assertEqual(kept, ["DLLs/_ssl.pyd", "Lib/os.py", "python.exe", "python312.dll"])

    def test_the_android_copy(self):
        # The games and the Python program are the APK's libraries: the copy carries a PYTHONHOME.
        import sys
        with tempfile.TemporaryDirectory() as tmp:
            home, out = Path(tmp) / "python", Path(tmp) / "openrac"
            (home / "lib/python3.13/lib-dynload").mkdir(parents=True)
            (home / "lib/python3.13/os.py").write_text("")
            (home / "lib/python3.13/test").mkdir()
            argv, sys.argv = sys.argv, ["bundle_standalone.py", str(out), "--android", "--python", str(home)]
            try:
                self.assertEqual(bundle.main(), 0)
            finally:
                sys.argv = argv
            self.assertTrue((out / "python/lib/python3.13/os.py").is_file())
            self.assertFalse((out / "python/lib/python3.13/test").exists())
            # No game program (the APK's libraries); the probe baker's Python half for Enhanced
            # graphics, whose program is a library of the APK too.
            self.assertFalse((out / "port/build").exists())
            self.assertFalse((out / "port/enhanced/tools/baker").exists())
            self.assertTrue((out / "port/enhanced/lighting/profiles/rac1-ntsc/level_01/probe-bake.json").is_file())
            self.assertIn("Android", (out / "STANDALONE").read_text())


if __name__ == "__main__":
    unittest.main()
