# SPDX-License-Identifier: GPL-3.0-or-later
"""The application descriptor follows the catalogue instead of duplicating disc constants."""
import json
from pathlib import Path
import tempfile
import unittest
from bundle import configuration
from package_python import install_launcher

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

    def test_store_artwork_is_explicit_and_omits_desktop_images(self):
        self.assertFalse(configuration(ROOT, "rac1-ntsc")["storeArtwork"])
        self.assertTrue(configuration(ROOT, "rac1-ntsc", store_artwork=True)["storeArtwork"])
        root = Path(tempfile.mkdtemp(prefix="openrac-store-art-"))
        dist = root / "launcher/dist"
        (dist / "img/store").mkdir(parents=True)
        (dist / "assets").mkdir()
        (dist / "img/rac1-thumb.webp").write_bytes(b"desktop image fixture")
        (dist / "img/store/rac1.svg").write_text("<svg/>")
        (dist / "assets/app.js").write_text("app")
        (dist / "assets/mock-test.js").write_text("mock")
        app = root / "store.app"
        install_launcher(app, root, True)
        self.assertFalse((app / "launcher-dist/img/rac1-thumb.webp").exists())
        self.assertTrue((app / "launcher-dist/img/store/rac1.svg").is_file())
        self.assertTrue((app / "launcher-dist/assets/app.js").is_file())
        self.assertFalse((app / "launcher-dist/assets/mock-test.js").exists())
        # A previously built desktop/dev bundle must not silently retain game photographs.
        (app / "launcher-dist/img/stale.webp").write_bytes(b"stale fixture")
        with self.assertRaisesRegex(RuntimeError, "prior desktop images"):
            install_launcher(app, root, True)
        dev = root / "dev.app"
        install_launcher(dev, root, False)
        self.assertTrue((dev / "launcher-dist/img/rac1-thumb.webp").is_file())

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
