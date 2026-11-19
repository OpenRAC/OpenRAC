"""tools/facts.py: packing, checking and fetching decomp build facts."""
from __future__ import annotations

import io
import json
import os
import subprocess
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import facts  # noqa: E402

DECOMP = "rac2/decomp"


def git(cwd: Path, *args: str) -> str:
    return subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", *args], cwd=cwd,
                          check=True, capture_output=True, text=True).stdout.strip()


class FactsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(lambda: __import__("shutil").rmtree(self.tmp, ignore_errors=True))
        self.src = self.tmp / "decomp"
        self.src.mkdir()
        git(self.src, "init", "-q")
        (self.src / "README").write_text("x\n")
        git(self.src, "add", "README")
        git(self.src, "commit", "-qm", "x")
        self.commit = git(self.src, "rev-parse", "HEAD")
        self.write_build(self.src)
        self.store = self.tmp / "store"
        self.store.mkdir()
        self.cache = self.tmp / "cache"
        env = mock.patch.dict(os.environ, {"OPENRAC_FACTS_URL": self.store.as_uri()})
        env.start()
        self.addCleanup(env.stop)

    def write_build(self, root: Path) -> None:
        placed = {"program": "boot", "functions": {"func_00115200": ["1135104"]},
                  "differ": {}, "values": {"D_00100080": "0x00100080"}}
        for rel in ("build/pal/boot/placed.json", "build/pal/00_aranos_tutorial/placed.json"):
            (root / rel).parent.mkdir(parents=True, exist_ok=True)
            (root / rel).write_text(json.dumps(placed))
        (root / "build/pal/carry.json").write_text(json.dumps(
            {"boot": {"func_00115200": {"address": 1135104, "size": 12, "how": "same", "was": "func_00115200"}}}))

    def publish_local(self, name: str, data: bytes) -> None:
        (self.store / name).write_bytes(data)

    def test_round_trip(self):
        name, data = facts.pack(DECOMP, "pal", self.src)
        self.assertEqual(name, f"rac2-decomp-{self.commit}-pal.zip")
        self.assertEqual(data, facts.pack(DECOMP, "pal", self.src)[1], "the same build packs the same bytes")
        self.publish_local(name, data)
        got = facts.fetch(DECOMP, "pal", self.commit, self.cache)
        for rel in ("build/pal/boot/placed.json", "build/pal/carry.json", "build/pal/00_aranos_tutorial/placed.json"):
            self.assertEqual((got / rel).read_bytes(), (self.src / rel).read_bytes())
        manifest = json.loads((got / "manifest.json").read_text())
        self.assertEqual((manifest["commit"], manifest["version"], manifest["decomp"]), (self.commit, "pal", DECOMP))

    def test_missing_bundle(self):
        with self.assertRaisesRegex(facts.FactsError, "build and publish facts for rac2/decomp"):
            facts.fetch(DECOMP, "pal", self.commit, self.cache)

    def test_wrong_sha256(self):
        name, data = facts.pack(DECOMP, "pal", self.src)
        z_in = zipfile.ZipFile(io.BytesIO(data))
        buf = io.BytesIO()
        with zipfile.ZipFile(buf, "w") as z:
            for n in z_in.namelist():
                body = z_in.read(n)
                z.writestr(n, body.replace(b"1135104", b"1135108") if n.endswith("carry.json") else body)
        self.publish_local(name, buf.getvalue())
        with self.assertRaisesRegex(facts.FactsError, "does not match its sha256"):
            facts.fetch(DECOMP, "pal", self.commit, self.cache)

    def test_wrong_commit(self):
        name, data = facts.pack(DECOMP, "pal", self.src)
        other = "0" * 40
        self.publish_local(facts.asset_name(DECOMP, other, "pal"), data)
        with self.assertRaisesRegex(facts.FactsError, "manifest says commit"):
            facts.fetch(DECOMP, "pal", other, self.cache)

    def test_cache_is_checked_again(self):
        name, data = facts.pack(DECOMP, "pal", self.src)
        self.publish_local(name, data)
        got = facts.fetch(DECOMP, "pal", self.commit, self.cache)
        (got / "build/pal/carry.json").write_text("{}")
        got = facts.fetch(DECOMP, "pal", self.commit, self.cache)
        self.assertEqual((got / "build/pal/carry.json").read_bytes(), (self.src / "build/pal/carry.json").read_bytes())

    def test_refuses_bytes(self):
        (self.src / "build/pal/carry.json").write_text(json.dumps({"boot": {"x": "27bdffd0afbf0020afb00010"}}))
        with self.assertRaisesRegex(facts.FactsError, "not a name, a number or a hash"):
            facts.pack(DECOMP, "pal", self.src)
        (self.src / "build/pal/carry.json").write_text(json.dumps({"boot": {"x": "a b\nc"}}))
        with self.assertRaises(facts.FactsError):
            facts.pack(DECOMP, "pal", self.src)

    def test_refuses_changed_checkout(self):
        (self.src / "README").write_text("changed\n")
        with self.assertRaisesRegex(facts.FactsError, "has changes"):
            facts.pack(DECOMP, "pal", self.src)

    def test_local_build_wins(self):
        self.assertEqual(facts.root(self.src, DECOMP, "pal"), self.src)
        bare = self.tmp / "bare"
        git(self.tmp, "clone", "-q", str(self.src), str(bare))
        name, data = facts.pack(DECOMP, "pal", self.src)
        self.publish_local(name, data)
        with mock.patch.object(facts, "CACHE", self.cache):
            got = facts.root(bare, DECOMP, "pal")
        self.assertTrue(got.is_relative_to(self.cache))
        self.assertTrue((got / "build/pal/carry.json").is_file())

    def test_specs_match_rac2_releases(self):
        """Each rac2 release's build folder (port/game/*/release/release.json) is its spec's."""
        for path in sorted((facts.ROOT / "port/game").glob("*/release/release.json")):
            info = json.loads(path.read_text())
            if info.get("game") == "rac2":
                self.assertEqual(facts.spec(DECOMP, info["version"])["build"], info["build"], path)
        self.assertEqual(facts.spec(DECOMP, facts.PRIMARY[DECOMP])["build"], "build")


if __name__ == "__main__":
    unittest.main()
