# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Tests for stub_calls.py: calls from generated C to functions with no C.

    python3 -m unittest port/tools/hostgen/test_stub_calls.py
"""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import stub_calls

STUBS = """#include "openrac/guest.h"

void func_V1_L05_0037D6E8(void) {
    static openrac_reach_site site_ = {"func_V1_L05_0037D6E8", "no-c", 0, 0, 0};
    openrac_guest_reach(&site_);
}

float game_PH_G1(gaddr a0) {
    static openrac_reach_site site_ = {"PH_G1", "no-c", 0, 0, 0};
    openrac_guest_reach(&site_);
    return 0;
}

void func_00100000(void) {
    static openrac_reach_site site_ = {"func_00100000", "asm", 0, 0, 0};
    openrac_guest_reach(&site_);
}
"""

GORN = """void func_V1_L05_0037D6E8(void);

void func_L15_00435780(gaddr a0)
{
    if (a0) {
        func_V1_L05_0037D6E8();
    }
    func_00100000();
}

static void func_L15_00435800(void) {
    float f = game_PH_G1(0);
    s->func_V1_L05_0037D6E8(f);
}
"""


class StubCalls(unittest.TestCase):
    def gen(self, root: Path) -> Path:
        (root / "levels").mkdir(parents=True)
        (root / "stubs.c").write_text(STUBS)
        (root / "levels" / "gorn.c").write_text(GORN)
        (root / "functions.c").write_text("{0x1u, (openrac_host_fn)func_V1_L05_0037D6E8, \"x\"},\n")
        return root

    def test_finds_calls_and_callers(self):
        with tempfile.TemporaryDirectory() as tmp:
            gen = self.gen(Path(tmp))
            names = stub_calls.stubs(gen)
            # Only "no-c" bodies: a function run as its assembly is not empty.
            self.assertEqual(names, {"func_V1_L05_0037D6E8", "game_PH_G1"})
            # The declaration, the field call and functions.c's address are not calls.
            self.assertEqual(stub_calls.calls(gen, names),
                             {("func_V1_L05_0037D6E8", "func_L15_00435780"): "levels/gorn.c",
                              ("game_PH_G1", "func_L15_00435800"): "levels/gorn.c"})

    def test_allow_table(self):
        with tempfile.TemporaryDirectory() as tmp:
            allow = Path(tmp) / "allow.tsv"
            allow.write_text("# name\tcaller\twhy\nf\tg\tnever reached\n")
            self.assertEqual(stub_calls.read_allow(allow), {("f", "g"): "never reached"})
            allow.write_text("f\tg\n")
            with self.assertRaises(SystemExit):
                stub_calls.read_allow(allow)


if __name__ == "__main__":
    unittest.main()
