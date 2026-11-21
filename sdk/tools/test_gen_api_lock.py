# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""gen_api_lock.py: the locks follow the headers, and only ever grow."""
from __future__ import annotations

import contextlib
import io
import json
import shutil
import tempfile
import unittest
from pathlib import Path

import gen_api_lock

HAS_CLANG = shutil.which(gen_api_lock.clang()) is not None

BASE = """#ifndef OPENRAC_MOD_H
#define OPENRAC_MOD_H
#include <stdint.h>
#define OPENRAC_MOD_API_MAJOR 1
#define OPENRAC_MOD_API_MINOR 2
#define OPENRAC_HOOK_OK 0
#define OPENRAC_G(api, a) ((void*)(a))
typedef struct openrac_mod openrac_mod;
typedef void (*openrac_frame_fn)(void* user);
typedef struct openrac_pose {
    uint32_t size;
    float position[3];
} openrac_pose;
typedef struct openrac_api {
    uint32_t size;
    void (*log)(openrac_mod* mod, int level, const char* text);
    int (*hero_pose)(openrac_pose* out);
} openrac_api;
#endif
"""


@unittest.skipUnless(HAS_CLANG, "needs clang")
class ApiLockTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.sdk = Path(self.tmp.name)
        (self.sdk / "include" / "openrac" / "games").mkdir(parents=True)
        self.write(BASE)
        (self.sdk / "include" / "openrac" / "games" / "rac1.h").write_text(
            '#include "openrac/openrac_mod.h"\n#define OPENRAC_RAC1_HOOK_Hero_Update "Hero_Update"\n')

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def write(self, text: str) -> None:
        (self.sdk / "include" / "openrac" / "openrac_mod.h").write_text(text)

    def run_tool(self, *args: str) -> tuple[int, str]:
        err = io.StringIO()
        with contextlib.redirect_stderr(err), contextlib.redirect_stdout(io.StringIO()):
            rc = gen_api_lock.main(["--sdk", str(self.sdk), *args])
        return rc, err.getvalue()

    def test_writes_and_checks(self) -> None:
        self.assertEqual(self.run_tool(), (0, ""))
        api = (self.sdk / "api" / "openrac_api.lock").read_text()
        self.assertIn("1\tlog\t8\tvoid (*)(openrac_mod *, int, const char *)", api)
        self.assertIn("2\thero_pose\t16\t", api)
        types = (self.sdk / "api" / "types.lock").read_text()
        self.assertIn("openrac_pose\t(size)\t16\t-", types)
        self.assertIn("openrac_pose\tposition\t4\tfloat[3]", types)
        self.assertIn("openrac_frame_fn\t(typedef)\t-\tvoid (*)(void *)", types)
        self.assertNotIn("uint32_t\t(typedef)", types)  # the system's own types are not the SDK's
        macros = (self.sdk / "api" / "macros.lock").read_text()
        self.assertIn("OPENRAC_G\thelper(api,a)", macros)
        self.assertIn('OPENRAC_RAC1_HOOK_Hero_Update\t"Hero_Update"', macros)
        self.assertEqual(self.run_tool("--check")[0], 0)

    def test_stale_lock_fails_check(self) -> None:
        self.run_tool()
        self.write(BASE.replace("} openrac_api;", "    int (*bolts)(int add);\n} openrac_api;"))
        rc, err = self.run_tool("--check")
        self.assertEqual(rc, 1)
        self.assertIn("openrac_api.lock is not what the headers give", err)
        self.assertEqual(self.run_tool()[0], 0)  # an added member is written
        self.assertIn("3\tbolts\t24\t", (self.sdk / "api" / "openrac_api.lock").read_text())

    def test_moved_member_refused(self) -> None:
        self.run_tool()
        moved = BASE.replace(
            "    void (*log)(openrac_mod* mod, int level, const char* text);\n"
            "    int (*hero_pose)(openrac_pose* out);\n",
            "    int (*hero_pose)(openrac_pose* out);\n"
            "    void (*log)(openrac_mod* mod, int level, const char* text);\n")
        self.write(moved)
        rc, err = self.run_tool()
        self.assertEqual(rc, 1)
        self.assertIn("member 1 was", err)

    def test_changed_signature_refused(self) -> None:
        self.run_tool()
        self.write(BASE.replace("int (*hero_pose)(openrac_pose* out)", "int (*hero_pose)(openrac_pose* out, int n)"))
        self.assertEqual(self.run_tool()[0], 1)

    def test_struct_field_appended_but_not_inserted(self) -> None:
        self.run_tool()
        self.write(BASE.replace("    float position[3];\n", "    float position[3];\n    float yaw;\n"))
        self.assertEqual(self.run_tool()[0], 0)
        self.write(BASE.replace("    float position[3];\n", "    float yaw;\n    float position[3];\n    float yaw2;\n"))
        rc, err = self.run_tool()
        self.assertEqual(rc, 1)
        self.assertIn("openrac_pose", err)

    def test_macros(self) -> None:
        self.run_tool()
        self.write(BASE.replace("OPENRAC_MOD_API_MINOR 2", "OPENRAC_MOD_API_MINOR 3"))
        self.assertEqual(self.run_tool()[0], 0)  # the minor moves
        self.write(BASE.replace("OPENRAC_HOOK_OK 0", "OPENRAC_HOOK_OK 1"))
        self.assertEqual(self.run_tool()[0], 1)
        self.write(BASE.replace("#define OPENRAC_HOOK_OK 0\n", ""))
        rc, err = self.run_tool()
        self.assertEqual(rc, 1)
        self.assertIn("is gone", err)

    def test_a_written_down_fix_may_change_its_row(self) -> None:
        self.run_tool()
        self.write(BASE.replace("OPENRAC_HOOK_OK 0", "OPENRAC_HOOK_OK 2"))
        self.assertEqual(self.run_tool()[0], 1)
        fix = {"lock": "macros.lock", "was": "OPENRAC_HOOK_OK\t0", "is": "OPENRAC_HOOK_OK\t2", "why": "a test"}
        (self.sdk / "api" / "fixes.json").write_text(json.dumps({"fixes": [fix]}))
        self.assertEqual(self.run_tool(), (0, ""))
        self.write(BASE.replace("OPENRAC_HOOK_OK 0", "OPENRAC_HOOK_OK 3"))  # not the fix written down
        self.assertEqual(self.run_tool()[0], 1)
        (self.sdk / "api" / "fixes.json").write_text(json.dumps({"fixes": [dict(fix, why="")]}))
        with self.assertRaises(SystemExit):
            self.run_tool()


@unittest.skipUnless(HAS_CLANG, "needs clang")
class RealSdkTest(unittest.TestCase):
    def test_committed_locks_are_current(self) -> None:
        with contextlib.redirect_stderr(io.StringIO()) as err:
            rc = gen_api_lock.main(["--check"])
        self.assertEqual(rc, 0, err.getvalue())


if __name__ == "__main__":
    unittest.main()
