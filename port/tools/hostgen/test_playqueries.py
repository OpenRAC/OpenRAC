# SPDX-License-Identifier: GPL-3.0-or-later
import json
import unittest
import tempfile
from pathlib import Path
import playqueries
import release

ROOT=Path(__file__).resolve().parents[3]
class QueryPoints(unittest.TestCase):
    def test_all_twenty_points_cover_each_disc(self):
        source=ROOT/"games/rac2/decomp"
        if not (source/"src").is_dir():self.skipTest("needs the pinned RAC2 source checkout")
        catalogue=ROOT/"port/game/rac2/play_queries.json"
        for version in ("ntsc","pal","ntsc2"):
            with self.subTest(version=version):
                view=None if version=="ntsc" else release.Release.load(ROOT/f"port/game/rac2-{version}/release/release_names.json")
                bodies=playqueries.build(source,catalogue,version,view)
                self.assertEqual(len(bodies),20)
                for body in bodies.values():
                    self.assertIn("openrac_mod_play_query(",body)
                    self.assertNotIn("[33]==169",body)
                    self.assertNotIn("[40]&",body)
                extras=next(body for body in bodies.values() if ".text_id = openrac_mod_play_query(16u" in body)
                self.assertIn({"ntsc":"0x3202","pal":"0x1114","ntsc2":"0x3098"}[version],extras)
    def test_source_drift_is_rejected(self):
        with self.assertRaises(ValueError):
            playqueries.instrument("RESET_MOBY_FUNCS","int test(int reload) { return reload; }","test","ntsc")

    def test_definition_hash_drift_is_rejected(self):
        source=ROOT/"games/rac2/decomp"
        if not (source/"src").is_dir():self.skipTest("needs the pinned RAC2 source checkout")
        data=json.loads((ROOT/"port/game/rac2/play_queries.json").read_text())
        next(iter(data["points"].values()))["ntsc"]["definition_sha256"]="0"*64
        with tempfile.TemporaryDirectory() as folder:
            catalogue=Path(folder)/"queries.json"
            catalogue.write_text(json.dumps(data))
            with self.assertRaisesRegex(ValueError,"source changed"):
                playqueries.build(source,catalogue,"ntsc")

    def test_race_entry_query_preserves_the_enclosing_condition(self):
        source="void sample(void *controller) { if ((g_race_challenge_table.entry[0].flags&0x10000000) || other()) done=1; }"
        body,count=playqueries.instrument("START_HOVERBIKE_CHALLENGE_FUNCS",source,"sample","ntsc")
        self.assertEqual(count,1)
        self.assertIn("if ((openrac_mod_play_query(",body)
        self.assertIn("|| other())",body)
