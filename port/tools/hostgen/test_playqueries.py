# SPDX-License-Identifier: GPL-3.0-or-later
import json
import unittest
import tempfile
import shutil
import subprocess
import sys
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
    def test_spaceship_reward_indices_remain_numeric_on_every_disc(self):
        source=ROOT/"games/rac2/decomp"
        if not (source/"src").is_dir():self.skipTest("needs the pinned RAC2 source checkout")
        catalogue=ROOT/"port/game/rac2/play_queries.json"
        data=json.loads(catalogue.read_text())
        for version in ("ntsc","pal","ntsc2"):
            with self.subTest(version=version):
                bodies=playqueries.build(source,catalogue,version)
                body=bodies[data["points"]["SPACESHIP_MENU_FUNCS"][version]["symbol"]]
                for record in ("D_L05_001B1350_gp[0]","g_feltzin_challenge_data[1]","g_feltzin_challenge_data[2]"):
                    self.assertIn("index = " + record + ";",body)
                self.assertEqual(body.count("openrac_mod_play_query(1u"),6)

    def test_spaceship_queries_preserve_reward_two_with_and_without_override(self):
        compiler=shutil.which("cc") or shutil.which("clang")
        if compiler is None:self.skipTest("needs a native C compiler")
        for explicit in (False,True):
            with self.subTest(explicit_comparisons=explicit), tempfile.TemporaryDirectory() as folder:
                a="D_L05_001B1350_gp[0]";b="g_feltzin_challenge_data[1]";c="g_feltzin_challenge_data[2]"
                condition=lambda value:value+" != 0" if explicit else value
                source=f"""int panel(unsigned int controller) {{
                    int index = {a}; int reward = rewards[index];
                    if ({condition(a)}) {{ index = {b}; reward += rewards[index]; }}
                    if ({condition(a)} && {condition(b)}) {{ index = {c}; reward += rewards[index]; }}
                    if ({condition(a)} && {condition(b)} && {condition(c)}) {{ index = g_feltzin_challenge_data[3]; reward += rewards[index]; }}
                    return reward;
                }}"""
                body,count=playqueries.instrument("SPACESHIP_MENU_FUNCS",source,"panel","ntsc")
                self.assertEqual(count,6)
                fixture="""#include <assert.h>
                    int D_L05_001B1350_gp[1], g_feltzin_challenge_data[4];
                    int rewards[3]={100,200,300}; int allow;
                    int openrac_mod_play_query(unsigned int kind,unsigned int subject,int index,int original,int alternate) {
                        return allow ? alternate : original;
                    }
                """+body+"""
                    int main(void) {
                        D_L05_001B1350_gp[0]=2;
                        g_feltzin_challenge_data[1]=g_feltzin_challenge_data[2]=g_feltzin_challenge_data[3]=2;
                        allow=0; assert(panel(0)==1200);
                        allow=1; assert(panel(0)==1200);
                        D_L05_001B1350_gp[0]=0;
                        allow=0; assert(panel(0)==100);
                        allow=1; assert(panel(0)==1000);
                        return 0;
                    }
                """
                root=Path(folder);path=root/"reward.c";path.write_text(fixture)
                exe=root/("reward.exe" if sys.platform=="win32" else "reward")
                subprocess.run([compiler,"-std=c11",str(path),"-o",str(exe)],check=True,capture_output=True)
                subprocess.run([str(exe)],check=True,capture_output=True)

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
