"""Reviewed material identities survive renaming; explicit overrides and ordinary surfaces remain intact."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from enhanced.emission import apply_materials, resolve_materials


class EmissionTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.level = self.root/'level'
        (self.level/'textures').mkdir(parents=True)
        (self.level/'textures/renamed.png').write_bytes(b'synthetic light texture')
        (self.level/'textures/wall.png').write_bytes(b'synthetic ordinary texture')
        self.catalog = self.root/'catalog.json'
        self.definition = dict(runtime_intensity=2, diffuse_scale=0)
        self.material = dict(name='fixture lamp',
            texture_sha256=hashlib.sha256(b'synthetic light texture').hexdigest(), emission=self.definition)
        self.write_catalog()

    def write_catalog(self):
        self.catalog.write_text(json.dumps(dict(version=1, materials=[self.material])))

    def test_content_match_ignores_filename_without_classifying_ordinary_textures(self):
        self.assertEqual(resolve_materials(self.level, self.catalog),
                         {'textures/renamed.png': self.definition})
        (self.level/'textures/another_name.png').write_bytes(b'synthetic light texture')
        self.assertEqual(len(resolve_materials(self.level, self.catalog)), 2)

    def test_catalog_updates_refresh_defaults_and_preserve_explicit_overrides(self):
        path = self.level/'probe-emission.json'
        apply_materials(self.level, self.catalog)
        self.definition['runtime_intensity'] = 3
        self.write_catalog()
        apply_materials(self.level, self.catalog)
        self.assertEqual(json.loads(path.read_text())['textures/renamed.png']['runtime_intensity'], 3)
        data = json.loads(path.read_text())
        data['textures/renamed.png']['runtime_intensity'] = 0
        path.write_text(json.dumps(data))
        self.definition['runtime_intensity'] = 4
        self.write_catalog()
        apply_materials(self.level, self.catalog)
        self.assertEqual(json.loads(path.read_text())['textures/renamed.png']['runtime_intensity'], 0)

    def test_removed_definition_cannot_leave_stale_generated_emission(self):
        path = self.level/'probe-emission.json'
        apply_materials(self.level, self.catalog)
        self.catalog.write_text('{"version":1,"materials":[]}')
        apply_materials(self.level, self.catalog)
        self.assertEqual(json.loads(path.read_text()), {})

    def test_partial_bake_override_keeps_shared_runtime_definition(self):
        path = self.level/'probe-emission.json'
        path.write_text('{"textures/renamed.png":{"bake_intensity":4}}')
        apply_materials(self.level, self.catalog)
        self.definition['runtime_intensity'] = 3
        self.write_catalog()
        apply_materials(self.level, self.catalog)
        material = json.loads(path.read_text())['textures/renamed.png']
        self.assertEqual(material['runtime_intensity'], 3)
        self.assertEqual(material['bake_intensity'], 4)

    def test_interrupted_staging_does_not_block_material_updates(self):
        stale = self.level/'probe-emission.json.part'
        stale.write_text('interrupted write')
        apply_materials(self.level, self.catalog)
        self.assertEqual(json.loads((self.level/'probe-emission.json').read_text())
                         ['textures/renamed.png']['runtime_intensity'], 2)
        self.assertEqual(stale.read_text(), 'interrupted write')
        self.assertFalse(list(self.level.glob('.probe-emission-*')))

    def test_failed_publish_preserves_profile_and_cleans_owned_staging(self):
        profile = self.level/'probe-emission.json'
        profile.write_text('{}')
        with patch.object(Path, 'replace', side_effect=OSError('publish failed')):
            with self.assertRaisesRegex(OSError, 'publish failed'):
                apply_materials(self.level, self.catalog)
        self.assertEqual(profile.read_text(), '{}')
        self.assertFalse(list(self.level.glob('.probe-emission-*')))

    def test_removing_tracked_defaults_keeps_only_independent_authoring(self):
        defaults = {'static_mobys': [42], 'textures/renamed.png': {'bake_intensity': 8}}
        apply_materials(self.level, self.catalog, defaults)
        apply_materials(self.level, self.catalog)
        copied = json.loads((self.level/'probe-emission.json').read_text())
        self.assertNotIn('static_mobys', copied)
        self.assertEqual(copied['textures/renamed.png'], self.definition)

    def test_invalid_or_duplicate_identities_fail(self):
        self.catalog.write_text(json.dumps(dict(version=1, materials=[self.material, self.material])))
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            resolve_materials(self.level, self.catalog)
        self.material['texture_sha256'] = 'not a digest'
        self.write_catalog()
        with self.assertRaisesRegex(ValueError, 'SHA-256'):
            resolve_materials(self.level, self.catalog)

    def test_malformed_catalog_types_report_configuration_errors(self):
        for catalog in ([], {'version': 1, 'materials': [None]},
                        {'version': 1, 'materials': [{'texture_sha256': 7}]}):
            with self.subTest(catalog=catalog):
                self.catalog.write_text(json.dumps(catalog))
                with self.assertRaises(ValueError):
                    resolve_materials(self.level, self.catalog)
