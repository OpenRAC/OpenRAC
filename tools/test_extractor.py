"""tools/extractor.py on synthetic disc images only, never a real disc:
python3 -m unittest discover -s tools"""

import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import extractor
from openrac_common import disc_files
from test_openrac import image

BOOT = b"\x7fELF" + bytes(3000)
CNF = b"BOOT2 = cdrom0:\\SCES_999.99;1\r\nVER = 1.00\r\n"


def fake_builds(boot_sha1: str | None, boot_sha256: str | None = None):
    """A game.json that knows one build, SCES_999.99 of game "rac9", by its boot's SHA-1 (or SHA-256 alone)."""
    game = {"id": "rac9", "title": "Nine"}
    boot = {"sha1": boot_sha1} if boot_sha1 else {"sha256": boot_sha256}
    version = {"serial": "SCES_999.99", "region": "PAL", "boot": boot, "disc": {}}
    return lambda wanted=None: {"SCES_999.99": [(game, "pal", version)]} if wanted in (None, "rac9") else {}


class ExtractTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.iso = self.dir / "my disc.iso"
        self.iso.write_bytes(image({"SYSTEM.CNF": CNF, "SCES_999.99": BOOT, "IOPRP.IMG": b"iop"}))
        self.proj = self.dir / "proj"

    def tearDown(self):
        self.tmp.cleanup()

    def run_extract(self, sha1=None):
        sha1 = sha1 or hashlib.sha1(BOOT).hexdigest()
        with mock.patch.object(extractor, "builds", fake_builds(sha1)):
            return extractor.extract(self.iso, "rac9", self.proj, minimum=0)

    def test_extracts_and_validates_a_known_build(self):
        info = self.run_extract()
        out = self.proj / "iso_data" / "rac9"
        self.assertEqual((out / "SCES_999.99").read_bytes(), BOOT)
        self.assertEqual((out / "SYSTEM.CNF").read_bytes(), CNF)
        self.assertEqual((out / "disc.iso").read_bytes(), self.iso.read_bytes())
        written = json.loads((out / "buildinfo.json").read_text())
        self.assertEqual(written, [info])
        self.assertEqual(info["serial"], "SCES_999.99")
        self.assertEqual(info["version"], "rac9/pal")
        self.assertEqual(info["files"], 3)
        self.assertFalse((self.proj / "iso_data" / "_temp").exists())

    def test_the_sector_map_gives_every_sector_of_the_image_from_the_files(self):
        self.run_extract()
        out = self.proj / "iso_data" / "rac9"
        rows = disc_files.read_map(out)
        whole = self.iso.read_bytes()
        self.assertEqual(rows[0].first, 0)
        self.assertEqual(sum(r.sectors for r in rows) * disc_files.SECTOR, len(whole))
        rebuilt = bytearray()
        for row in rows:
            self.assertEqual(row.first * disc_files.SECTOR, len(rebuilt))
            data = (out / row.path).read_bytes()
            self.assertEqual(len(data), row.bytes)
            rebuilt += data + bytes(row.sectors * disc_files.SECTOR - row.bytes)
        self.assertEqual(bytes(rebuilt), whole)
        # The file system's files are rows of their own; the rest are lumps.
        self.assertIn("SCES_999.99", [r.path for r in rows])
        self.assertTrue(all(r.path.startswith("lumps/") for r in rows if r.path not in ("SYSTEM.CNF", "SCES_999.99", "IOPRP.IMG")))

    def test_lumps_are_cut_where_the_table_of_contents_points(self):
        files = [("A.BIN", 20, 100), ("B.BIN", 30, 5000)]
        rows = disc_files.plan(files, 50, {5: "toc", 40: "levels/00/level", 45: "global/001"})
        self.assertEqual([(r.first, r.sectors, r.path) for r in rows], [
            (0, 5, "lumps/sectors/0000000.bin"), (5, 15, "lumps/toc.bin"), (20, 1, "A.BIN"),
            (21, 9, "lumps/sectors/0000021.bin"), (30, 3, "B.BIN"), (33, 7, "lumps/sectors/0000033.bin"),
            (40, 5, "lumps/levels/00/level.bin"), (45, 5, "lumps/global/001.bin")])
        self.assertEqual(rows[2].bytes, 100)

    def test_a_file_whose_padding_is_not_zeros_leaves_its_last_sector_to_a_lump(self):
        data = bytearray(self.iso.read_bytes())
        lba = next(l for p, l, _ in extractor.files_of(self.iso) if p == "IOPRP.IMG")
        data[lba * disc_files.SECTOR + 10] = 0x55
        self.iso.write_bytes(bytes(data))
        self.run_extract()
        out = self.proj / "iso_data" / "rac9"
        rows = {r.first: r for r in disc_files.read_map(out)}
        self.assertEqual(rows[lba].path, f"lumps/sectors/{lba:07}.bin")
        self.assertEqual((out / rows[lba].path).read_bytes()[10], 0x55)
        self.assertEqual((out / "IOPRP.IMG").read_bytes(), b"iop")

    def test_boot_only_writes_the_boot_executable_alone_and_checks_every_file(self):
        with mock.patch.object(extractor, "builds", fake_builds(hashlib.sha1(BOOT).hexdigest())):
            info = extractor.extract(self.iso, "rac9", self.proj, minimum=0, boot_only=True)
        out = self.proj / "iso_data" / "rac9"
        self.assertEqual((out / "SCES_999.99").read_bytes(), BOOT)
        self.assertEqual((out / "SYSTEM.CNF").read_bytes(), CNF)
        self.assertFalse((out / "IOPRP.IMG").exists())
        self.assertFalse((out / disc_files.MAP_NAME).exists())
        self.assertEqual((out / "disc.iso").read_bytes(), self.iso.read_bytes())
        self.assertEqual(info["files"], 3)
        self.assertEqual(info, self.run_extract())  # the same build info as a full extraction

    def test_link_image_keeps_a_symbolic_link_where_no_hard_link_can_be_made(self):
        # Android lets no app make hard links: the launcher's own copy is linked, not copied again.
        with mock.patch.object(extractor, "builds", fake_builds(hashlib.sha1(BOOT).hexdigest())), \
                mock.patch("os.link", side_effect=PermissionError("no hard links")):
            extractor.extract(self.iso, "rac9", self.proj, minimum=0, boot_only=True, link_image=True)
        image = self.proj / "iso_data" / "rac9" / "disc.iso"
        self.assertTrue(image.is_symlink())
        self.assertEqual(image.resolve(), self.iso.resolve())
        # Set up again from the same image: the new link replaces the old one, the image stays.
        with mock.patch.object(extractor, "builds", fake_builds(hashlib.sha1(BOOT).hexdigest())), \
                mock.patch("os.link", side_effect=PermissionError("no hard links")):
            extractor.extract(self.iso, "rac9", self.proj, minimum=0, boot_only=True, link_image=True)
        self.assertTrue(self.iso.is_file())
        self.assertEqual(image.read_bytes(), self.iso.read_bytes())

    def test_set_up_again_from_its_own_image_keeps_it(self):
        # Android before --link-image: the set-up's disc.iso is a copy. Set up from it again,
        # it moves into the new set-up instead of a link to a file that is then removed.
        with mock.patch.object(extractor, "builds", fake_builds(hashlib.sha1(BOOT).hexdigest())), \
                mock.patch("os.link", side_effect=PermissionError("no hard links")):
            extractor.extract(self.iso, "rac9", self.proj, minimum=0, boot_only=True)
            own = self.proj / "iso_data" / "rac9" / "disc.iso"
            info = extractor.extract(own, "rac9", self.proj, minimum=0, boot_only=True, link_image=True)
        self.assertTrue(own.is_file())
        self.assertFalse(own.is_symlink())
        self.assertEqual(own.read_bytes(), self.iso.read_bytes())
        self.assertEqual(info["image"], self.iso.name, "the name of the image it was first set up from")
        self.assertFalse((self.proj / "iso_data" / "_temp").exists())

    def test_without_link_image_a_failed_hard_link_copies(self):
        with mock.patch.object(extractor, "builds", fake_builds(hashlib.sha1(BOOT).hexdigest())), \
                mock.patch("os.link", side_effect=PermissionError("no hard links")):
            extractor.extract(self.iso, "rac9", self.proj, minimum=0, boot_only=True)
        image = self.proj / "iso_data" / "rac9" / "disc.iso"
        self.assertFalse(image.is_symlink())
        self.assertEqual(image.read_bytes(), self.iso.read_bytes())

    def test_a_second_extraction_replaces_the_first(self):
        self.run_extract()
        (self.proj / "iso_data" / "rac9" / "stale").write_text("x")
        self.run_extract()
        self.assertFalse((self.proj / "iso_data" / "rac9" / "stale").exists())

    def test_refuses_an_unknown_revision_and_keeps_nothing(self):
        with self.assertRaises(extractor.ExtractError) as caught:
            self.run_extract(sha1="0" * 40)
        self.assertEqual(caught.exception.code, 4002)
        self.assertFalse((self.proj / "iso_data" / "rac9").exists())
        self.assertFalse((self.proj / "iso_data" / "_temp").exists())

    def test_a_build_pinned_by_sha256_alone(self):
        good = hashlib.sha256(BOOT).hexdigest()
        with mock.patch.object(extractor, "builds", fake_builds(None, good)):
            self.assertEqual(extractor.extract(self.iso, "rac9", self.proj, minimum=0)["serial"], "SCES_999.99")
        with mock.patch.object(extractor, "builds", fake_builds(None, "0" * 64)):
            with self.assertRaises(extractor.ExtractError) as caught:
                extractor.extract(self.iso, "rac9", self.proj, minimum=0)
        self.assertEqual(caught.exception.code, 4002)

    def test_knows_the_rac2_pal_disc(self):
        # The European R&C2 disc is a known build.
        [(_, name, version)] = extractor.builds("rac2")["SCES_516.07"]
        self.assertEqual(name, "pal")
        self.assertEqual(version["boot"]["sha1"], "cb59ce5396ddebb8aa539b0847f8bf66d6a47fe0")
        self.assertEqual(version["disc"]["files"], 96)

    def test_one_serial_two_builds_told_apart_by_the_boot(self):
        # Going Commando's v1.01 and v2.00 both boot SCUS_972.68.
        found = {name: version["boot"]["sha1"] for _, name, version in extractor.builds("rac2")["SCUS_972.68"]}
        self.assertEqual(set(found), {"ntsc", "ntsc2"})
        game = {"id": "rac2", "title": "Going Commando"}
        both = {"SCUS_972.68": [(game, name, {"serial": "SCUS_972.68", "region": "NTSC-U", "boot": {"sha1": sha1},
                                              "disc": {}}) for name, sha1 in found.items()]}
        with mock.patch.object(extractor, "builds", lambda wanted=None: both):
            for name, sha1 in found.items():
                _, which, _ = extractor.validate("rac2", "SCUS_972.68", sha1, {"SCUS_972.68": sha1})
                self.assertEqual(which, name)
            with self.assertRaises(extractor.ExtractError) as caught:
                extractor.validate("rac2", "SCUS_972.68", "0" * 40, {"SCUS_972.68": "0" * 40})
        self.assertEqual(caught.exception.code, 4002)

    def test_ratchet_and_clanks_pal_releases_share_a_serial_and_japan_has_its_own(self):
        # PAL v1.00 and v2.00 both boot SCES_509.16; the Japanese release is SCPS_150.37.
        found = {name: version["boot"]["sha1"] for _, name, version in extractor.builds("rac1")["SCES_509.16"]}
        self.assertEqual(found, {"pal": "79956931bd62fafd8d20fa2eae796dbaf2e15e83",
                                 "pal1": "553a7f6c2c7a004d384d77b7139accbc52242149"})
        game = {"id": "rac1", "title": "Ratchet & Clank"}
        both = {"SCES_509.16": [(game, name, {"serial": "SCES_509.16", "region": "PAL", "boot": {"sha1": sha1},
                                              "disc": {}}) for name, sha1 in found.items()]}
        with mock.patch.object(extractor, "builds", lambda wanted=None: both):
            for name, sha1 in found.items():
                _, which, _ = extractor.validate("rac1", "SCES_509.16", sha1, {"SCES_509.16": sha1})
                self.assertEqual(which, name)
            with self.assertRaises(extractor.ExtractError) as caught:
                extractor.validate("rac1", "SCES_509.16", "0" * 40, {"SCES_509.16": "0" * 40})
        self.assertEqual(caught.exception.code, 4002)
        [(_, name, jp)] = extractor.builds("rac1")["SCPS_150.37"]
        self.assertEqual((name, jp["region"], jp["boot"]["sha1"]), ("jp", "NTSC-J (Japan)",
                                                                   "0df83877d53e19d69ec82038e2aaa227f643178f"))
        self.assertEqual(jp["disc"]["files"], 3)

    def test_refuses_another_game(self):
        with mock.patch.object(extractor, "builds", fake_builds(hashlib.sha1(BOOT).hexdigest())):
            with self.assertRaises(extractor.ExtractError) as caught:
                extractor.extract(self.iso, "rac8", self.proj, minimum=0)
        self.assertEqual(caught.exception.code, 4001)

    def test_refuses_what_is_not_a_disc_image(self):
        cases = [(self.dir / "notes.txt", b"hello", 4040), (self.dir / "blank.iso", bytes(40 * 2048), 4020)]
        for path, data, code in cases:
            path.write_bytes(data)
            with self.assertRaises(extractor.ExtractError) as caught:
                extractor.extract(path, "rac9", self.proj, minimum=0)
            self.assertEqual(caught.exception.code, code, path.name)
        with self.assertRaises(extractor.ExtractError) as caught:
            extractor.extract(self.iso, "rac9", self.proj)
        self.assertEqual(caught.exception.code, 4041)

    def test_every_refusal_of_the_disc_reader_is_error_4020_with_its_reason(self):
        self.assertEqual([path for path, _lba, _size in extractor.files_of(self.iso)],
                         ["SYSTEM.CNF", "SCES_999.99", "IOPRP.IMG"])
        cut = bytearray(image({"SYSTEM.CNF": CNF}))
        cut[16 * 2048 + 156 + 10:16 * 2048 + 156 + 14] = (2 * 34 + 20).to_bytes(4, "little")
        for data, detail in ((bytes(40 * 2048), ""),
                             (image({"A B.BIN": b"x"}), "a file name OpenRAC will not write: 'A B.BIN'"),
                             (bytes(cut), "a directory record is cut short")):
            self.iso.write_bytes(data)
            with self.assertRaises(extractor.ExtractError) as caught:
                extractor.files_of(self.iso)
            self.assertEqual((caught.exception.code, caught.exception.detail), (4020, detail))

    def test_no_boot_executable(self):
        self.iso.write_bytes(image({"README.TXT": b"hello"}))
        with self.assertRaises(extractor.ExtractError) as caught:
            self.run_extract()
        self.assertEqual(caught.exception.code, 4000)

    def test_the_later_steps_do_not_exist_yet(self):
        code = extractor.main([str(self.iso), "--game", "rac1", "--decompile", "--proj-path", str(self.proj)])
        self.assertEqual(code, 1)

    def test_contents_hash_ignores_order(self):
        a = extractor.contents_hash({"A": "1", "B": "2"})
        self.assertEqual(a, extractor.contents_hash({"B": "2", "A": "1"}))
        self.assertNotEqual(a, extractor.contents_hash({"A": "1", "B": "3"}))

    def test_every_real_build_is_known(self):
        serials = extractor.builds()
        for serial in ("SCES_509.16", "SCUS_971.99", "SCPS_150.37", "SCUS_972.68", "SCES_516.07", "SCUS_973.53", "SCES_524.56", "SCUS_974.65", "SCES_532.85"):
            self.assertIn(serial, serials)
            for _game, _name, version in serials[serial]:
                self.assertEqual(len(version["boot"]["sha1"]), 40)
        # Size Matters' project pins its boot by SHA-256 only.
        self.assertEqual(len(serials["SCUS_976.15"][0][2]["boot"]["sha256"]), 64)


if __name__ == "__main__":
    unittest.main()
