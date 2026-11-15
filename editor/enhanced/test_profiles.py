"""Exact game/version/level selection and independent profile copies, without disc data."""

import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

from enhanced import profiles


class ProfileTests(unittest.TestCase):
    def test_going_commandos_discs_share_v101s_profiles(self):
        self.assertEqual([profiles.profile_name("rac2", v) for v in ("ntsc", "pal", "ntsc2")], ["rac2-ntsc"] * 3)
        self.assertEqual(profiles.profile_name("rac1", "pal"), "rac1-pal")
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            profiles.write_bake_profile(SimpleNamespace(game="rac2", version="ntsc2", id=5), directory)
            shared = profiles.PROFILE_ROOT / "rac2-ntsc/level_05/probe-bake.json"
            self.assertEqual((directory / "probe-bake.json").read_bytes(), shared.read_bytes())

    def test_object_lights_shared_by_pal_and_ntsc(self):
        """Model-attached lights share definitions across revisions and reach standalone exports."""
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root/'profiles/rac1/object-lights.json'
            source.parent.mkdir(parents=True)
            source.write_text('{"version":1,"objects":{"mobys/moby_601.glb":[]}}')
            for version in ('ntsc', 'pal'):
                output = root/version
                output.mkdir()
                with mock.patch.object(profiles,'PROFILE_ROOT',root/'profiles'):
                    profiles.write_bake_profile(SimpleNamespace(game='rac1',version=version,id=1),output)
                self.assertEqual((output/'object-lights.json').read_bytes(),source.read_bytes())

    def test_foliage_shared_by_both_versions_without_replacing_source(self):
        """Only material metadata is copied; PAL and NTSC use the same content-based selections."""
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root/'profiles/rac1/foliage-materials.json'
            source.parent.mkdir(parents=True)
            source.write_text('{"version":1,"materials":[]}')
            for version in ('ntsc', 'pal'):
                output = root/version
                output.mkdir()
                with mock.patch.object(profiles, 'PROFILE_ROOT', root/'profiles'):
                    profiles.write_bake_profile(SimpleNamespace(game='rac1', version=version, id=1), output)
                target = output/'foliage-materials.json'
                self.assertEqual(target.read_bytes(), source.read_bytes())
                target.write_text('{}')
                self.assertEqual(source.read_text(), '{"version":1,"materials":[]}')

    def test_material_layers_keep_local_authoring_and_match_both_discs(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root/'profiles/rac1/material-overrides.json'
            source.parent.mkdir(parents=True)
            shared = '{"version":1,"presets":{"coat":{"wetness":{"reflectivity":0.04}}}}'
            source.write_text(shared)
            for version in ('pal', 'ntsc'):
                output = root/version
                output.mkdir()
                local = output/'material-overrides.json'
                local.write_text('{"version":1,"overrides":[]}')
                with mock.patch.object(profiles, 'PROFILE_ROOT', root/'profiles'):
                    profiles.write_bake_profile(SimpleNamespace(game='rac1', version=version, id=1), output)
                self.assertEqual((output/'materials/shared/material-overrides.json').read_text(), shared)
                self.assertEqual(json.loads(local.read_text()), {'version':1, 'overrides':[]})

    def test_background_scene_strategies_are_exported(self):
        """Each tracked background profile reaches exports without a gameplay dependency."""
        for number in (98, 99):
            with self.subTest(level=number), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                profiles.write_bake_profile(SimpleNamespace(game='rac1', version='ntsc', id=number), directory)
                settings = json.loads((directory/'probe-bake.json').read_text())
                self.assertEqual(settings['strategy'], 'scene')

    def test_authored_level_materials_survive_export_and_refresh(self):
        """Saved Studio definitions and assignments ship with their exact level, below local edits."""
        for number, material in ((1, 'lit-default'), (3, 'kerwan-building'), (4, 'ratchet-coat')):
            with self.subTest(level=number), tempfile.TemporaryDirectory() as temporary:
                output = Path(temporary)
                source = profiles.PROFILE_ROOT / f'rac1-ntsc/level_{number:02}/material-overrides.json'
                authored = json.loads(source.read_text())
                self.assertIn(material, authored['presets'])
                local = output / 'material-overrides.json'
                local_text = '{"version":1,"presets":{"my-material":{"type":"original"}}}'
                local.write_text(local_text)
                level = SimpleNamespace(game='rac1', version='ntsc', id=number)
                profiles.write_bake_profile(level, output)
                target = output / 'materials/level/material-overrides.json'
                self.assertEqual(json.loads(target.read_text()), authored)
                self.assertEqual(local.read_text(), local_text)
                target.write_text('{"version":1,"overrides":[]}')
                profiles.write_bake_profile(level, output)
                self.assertEqual(json.loads(target.read_text()), authored)
                self.assertEqual(local.read_text(), local_text)

    def test_generated_material_layer_waits_for_the_baker(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root/'profiles/rac1-ntsc/level_02'
            source.mkdir(parents=True)
            (source/'emission-textures.json').write_text('{"version":1,"textures":[]}')
            (source/'material-overrides.json').write_text('{"version":1,"overrides":[]}')
            export = root/'export'
            export.mkdir()
            with mock.patch.object(profiles, 'PROFILE_ROOT', root/'profiles'):
                profiles.write_material_profiles(SimpleNamespace(game='rac1', version='ntsc', id=2), export)
            self.assertFalse((export/'materials/level/material-overrides.json').exists())

    def test_region_settings_are_exported_with_bake_profile(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / 'profiles/rac1-ntsc/level_01'
            source.mkdir(parents=True)
            text = '{"version":1,"regions":{"coarse_bounds":"local_regions","coarse_padding":128}}'
            (source / 'probe-bake.json').write_text(text)
            directory = root / 'export'
            directory.mkdir()
            with mock.patch.object(profiles, 'PROFILE_ROOT', root / 'profiles'):
                profiles.write_bake_profile(SimpleNamespace(game='rac1', version='ntsc', id=1), directory)
            self.assertEqual((directory / 'probe-bake.json').read_text(), text)

    def test_blarg_lighting_environments_are_exported_for_ntsc_only(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            name = "lighting-environments.json"
            tracked = profiles.PROFILE_ROOT / "rac1-ntsc/level_06" / name
            for version in ("ntsc", "pal"):
                export = root / version
                export.mkdir()
                profiles.write_bake_profile(SimpleNamespace(game="rac1", version=version, id=6), export)
                if version == "ntsc":
                    self.assertEqual((export / name).read_bytes(), tracked.read_bytes())
                else:
                    self.assertFalse((export / name).exists())

    def test_only_the_exact_game_version_and_level_match(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "profiles/rac1-ntsc/level_01/probe-bake.json"
            source.parent.mkdir(parents=True)
            source.write_bytes(b'{"version":1,"sky_intensity":0.5}\n')
            water_source = root / "water/rac1-ntsc/level_01/water-boundaries.json"
            water_source.parent.mkdir(parents=True)
            water_source.write_bytes(b'{"open_edges":[]}\n')
            for game, version, number in (("rac1", "ntsc", 1), ("rac1", "pal", 1),
                                          ("rac2", "ntsc", 1), ("rac1", "ntsc", 2)):
                with self.subTest(game=game, version=version, level=number):
                    directory = root / f"{game}-{version}-{number}"
                    directory.mkdir()
                    with mock.patch.object(profiles, "PROFILE_ROOT", root / "profiles"), \
                            mock.patch.object(profiles, "WATER_PROFILE_ROOT", root / "water"):
                        profiles.write_bake_profile(SimpleNamespace(game=game, version=version, id=number), directory)
                    water_copy = directory / "water-boundaries.json"
                    self.assertEqual(water_copy.exists(), (game, version, number) == ("rac1", "ntsc", 1))
                    if water_copy.exists():
                        self.assertEqual(water_copy.read_bytes(), water_source.read_bytes())
                        water_copy.write_bytes(b'{}')
                        self.assertEqual(water_source.read_bytes(), b'{"open_edges":[]}\n')
                    copied = directory / "probe-bake.json"
                    self.assertEqual(copied.exists(), (game, version, number) == ("rac1", "ntsc", 1))
                    if copied.exists():
                        self.assertEqual(copied.read_bytes(), source.read_bytes())
                        copied.write_bytes(b'{"version":1}\n')
                        self.assertIn(b'"sky_intensity":0.5', source.read_bytes())

    def test_emission_profile_is_copied_with_independent_bake_settings(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "profiles/rac1-ntsc/level_01"
            source.mkdir(parents=True)
            emission = b'{"static_mobys":[42],"textures/lamp.png":{"runtime_intensity":2}}\n'
            (source / "probe-emission.json").write_bytes(emission)
            directory = root / "export"
            directory.mkdir()
            with mock.patch.object(profiles, "PROFILE_ROOT", root / "profiles"):
                profiles.write_bake_profile(SimpleNamespace(game="rac1", version="ntsc", id=1), directory)
            copied = json.loads((directory / "probe-emission.json").read_text())
            self.assertEqual(copied.pop('_catalog_defaults'), json.loads(emission))
            self.assertEqual(copied, json.loads(emission))
            self.assertFalse((directory / "probe-bake.json").exists())
            (directory / "probe-emission.json").write_text('{}')
            self.assertEqual((source / "probe-emission.json").read_bytes(), emission)

    def test_existing_export_refreshes_tracked_defaults_and_keeps_local_overrides(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root/'profiles/rac1-ntsc/level_01'
            source.mkdir(parents=True)
            profile = source/'probe-emission.json'
            profile.write_text('{"static_mobys":[42],"textures/lamp.png":{"runtime_intensity":2}}')
            directory = root/'export'
            directory.mkdir()
            exported = directory/'probe-emission.json'
            exported.write_text('{"textures/wall.png":{"runtime_intensity":0}}')
            level = SimpleNamespace(game='rac1', version='ntsc', id=1)
            with mock.patch.object(profiles, 'PROFILE_ROOT', root/'profiles'):
                profiles.write_emission_profile(level, directory)
                copied = json.loads(exported.read_text())
                self.assertEqual(copied['textures/lamp.png']['runtime_intensity'], 2)
                self.assertEqual(copied['static_mobys'], [42])
                copied['textures/lamp.png']['bake_intensity'] = 8
                exported.write_text(json.dumps(copied))
                profile.write_text('{"static_mobys":[42,43],"textures/lamp.png":{"runtime_intensity":3}}')
                profiles.write_emission_profile(level, directory)
            copied = json.loads(exported.read_text())
            self.assertEqual(copied['textures/lamp.png'], {'runtime_intensity':3, 'bake_intensity':8})
            self.assertEqual(copied['textures/wall.png']['runtime_intensity'], 0)
            self.assertEqual(copied['static_mobys'], [42,43])

    def test_copy_errors_abort_export_instead_of_using_different_settings(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "profiles/rac1-ntsc/level_01/probe-bake.json"
            source.parent.mkdir(parents=True)
            source.write_text('{"version":1}')
            with mock.patch.object(profiles, "PROFILE_ROOT", root / "profiles"), \
                    mock.patch.object(profiles.shutil, "copyfile", side_effect=OSError("cannot copy profile")):
                with self.assertRaisesRegex(OSError, "cannot copy profile"):
                    profiles.write_bake_profile(SimpleNamespace(game="rac1", version="ntsc", id=1), root)

    def test_water_copy_errors_abort_export(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "water/rac1-ntsc/level_01/water-boundaries.json"
            source.parent.mkdir(parents=True)
            source.write_text('{"open_edges":[]}')
            with mock.patch.object(profiles, "PROFILE_ROOT", root / "lighting"), \
                    mock.patch.object(profiles, "WATER_PROFILE_ROOT", root / "water"), \
                    mock.patch.object(profiles.shutil, "copyfile", side_effect=OSError("cannot copy water profile")):
                with self.assertRaisesRegex(OSError, "cannot copy water profile"):
                    profiles.write_bake_profile(SimpleNamespace(game="rac1", version="ntsc", id=1), root)


if __name__ == "__main__":
    unittest.main()
