# SPDX-License-Identifier: GPL-3.0-or-later
"""The application descriptor follows the catalogue instead of duplicating disc constants."""
import json
from pathlib import Path
import tempfile
import unittest
from bundle import configuration

ROOT = Path(__file__).resolve().parents[2]


class BundleTests(unittest.TestCase):
    def test_descriptor_uses_catalogue_disc_and_boot(self):
        actual = configuration(ROOT, "rac1-ntsc")
        catalogue = json.loads((ROOT / "games/rac1/game.json").read_text())["versions"]["ntsc"]
        engine = actual["engines"][0]
        self.assertEqual(engine["discSize"], catalogue["disc"]["size"])
        self.assertEqual(engine["bootSha1"], catalogue["boot"]["sha1"])
        self.assertEqual(engine["bootFile"], catalogue["boot"]["file"])
        self.assertEqual(actual["storeUrl"], "")

    def test_unknown_engine_and_non_store_update_pages_are_rejected(self):
        with self.assertRaises(ValueError):
            configuration(ROOT, "unknown-engine")
        with self.assertRaises(ValueError):
            configuration(ROOT, "rac1-ntsc", "https://example.com/update")

    def test_catalogue_changes_need_no_ios_constant_changes(self):
        root = Path(tempfile.mkdtemp(prefix="openrac-bundle-"))
        for relative in ("port/ios/engines.json", "games/rac1/game.json", "launcher/package.json"):
            target = root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((ROOT / relative).read_bytes())
        file = root / "games/rac1/game.json"
        catalogue = json.loads(file.read_text())
        catalogue["versions"]["ntsc"]["disc"]["size"] = 1234
        file.write_text(json.dumps(catalogue))
        self.assertEqual(configuration(root, "rac1-ntsc")["engines"][0]["discSize"], 1234)


if __name__ == "__main__":
    unittest.main()
