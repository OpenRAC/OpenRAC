# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""An emission audit reports authored brightness without assigning emission or changing sources."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from emission_audit import audit


class EmissionAudit(unittest.TestCase):
    def test_fullbright_candidates_are_reported_without_becoming_emitters(self):
        class Document:
            def list(self, key):
                return {
                    'meshes': [{'primitives': [{'attributes': {'POSITION': 0, 'COLOR_0': 1}}]}],
                    'materials': [{'pbrMetallicRoughness': {'baseColorTexture': {'index': 0}}}],
                    'textures': [{'source': 0}], 'images': [{'uri': 'textures/panel.png'}]}[key]

            def accessor(self, index):
                return [[0, 0, 0]]*3 if index == 0 else [[255/128]*3+[1]]*3

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            level, output = root/'level', root/'output'
            level.mkdir()
            output.mkdir()
            (level/'manifest.json').write_text('{"terrain":"terrain.glb","placements":"placements.json"}')
            (level/'placements.json').write_text('{}')
            with patch('gltf_read.read', return_value=Document()):
                report = audit(level, output)
                self.assertFalse(report['candidates'][0]['defined'])
                self.assertEqual(report['candidates'][0]['fullbright_triangles'], 1)
                self.assertFalse((level/'probe-emission.json').exists())
                (level/'probe-emission.json').write_text('{"textures/panel.png":{}}')
                reviewed = audit(level, output)
                self.assertTrue(reviewed['candidates'][0]['defined'])
                self.assertEqual(json.loads((level/'probe-emission.json').read_text()),
                                 {'textures/panel.png': {}})
