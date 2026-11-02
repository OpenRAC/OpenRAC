"""The opt-in -frac2-hazard-labels: transformer anchors and EE compiler/assembler regressions.

With the option, cc1 writes `.set rac2_hazard_labels' and gas keeps a label in front of the
coprocessor hazard nop it inserts after the label (tools/compiler/transformers/hazard_labels.py).
Ordinary CI runs the transformer tests. The output tests need the newly built tools: they skip
without them, and RAC2_COMPILER_ROOT naming a directory without them is an error.
"""
import importlib.util
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import elf  # noqa: E402

spec = importlib.util.spec_from_file_location("hazard_labels",
    ROOT / "tools/compiler/transformers/hazard_labels.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

DIRECTIVE = "\t.set\trac2_hazard_labels\n"
SUB_S = 0x46000001          # sub.s with its register fields cleared


def branch_targets(words):
    """{word index of a PC-relative branch: word index of its target}."""
    out = {}
    for i, word in enumerate(words):
        opcode = word >> 26
        if opcode in (1, 4, 5, 6, 7, 20, 21, 22, 23) or (opcode == 17 and (word >> 21) & 31 == 8):
            offset = word & 0xFFFF
            out[i] = i + 1 + (offset - 0x10000 if offset & 0x8000 else offset)
    return out


def functions(path):
    """{name: words} for every .text.<name> section of an object."""
    obj = elf.read_object(path)
    return {s["name"][len(".text."):]: [w for (w,) in struct.iter_unpack("<I", s["data"])]
            for s in obj["sections"] if s["name"].startswith(".text.")}


def anchored_sources():
    """Minimal sources holding every anchor the transformer edits."""
    return {"gas/config/tc-mips.c": transformer.STATE_OLD + transformer.APPEND_OLD + "\n"
                                    + transformer.DELAYS_OLD + transformer.SET_OLD,
            "gcc/flags.h": "extern int flag_rac2_qcopy;\n",
            "gcc/toplev.c": "int flag_rac2_qcopy = 0;\n" + '  {"rac2-qcopy", &flag_rac2_qcopy, 1,\n',
            "gcc/config/mips/mips.c": transformer.START_OLD}


def write_tree(root, files):
    for name, contents in files.items():
        (root / name).parent.mkdir(parents=True, exist_ok=True)
        (root / name).write_text(contents)


class TransformerTests(unittest.TestCase):
    def test_every_anchor_is_edited_once(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            write_tree(root, anchored_sources())
            transformer.patch(root)
            assembler = (root / "gas/config/tc-mips.c").read_text()
            self.assertIn("static int rac2_hazard_labels;", assembler)
            self.assertIn("(rac2_div_keep_labels || rac2_hazard_labels) ? NULL : insn_labels", assembler)
            self.assertIn("for (l = rac2_hazard_labels ? NULL : insn_labels;", assembler)
            self.assertIn('strcmp (name, "rac2_hazard_labels") == 0', assembler)
            self.assertIn("int flag_rac2_hazard_labels = 0;", (root / "gcc/toplev.c").read_text())
            self.assertIn('{"rac2-hazard-labels", &flag_rac2_hazard_labels, 1,',
                          (root / "gcc/toplev.c").read_text())
            self.assertIn("extern int flag_rac2_hazard_labels;", (root / "gcc/flags.h").read_text())
            self.assertIn('fprintf (stream, "\\t.set\\trac2_hazard_labels\\n");',
                          (root / "gcc/config/mips/mips.c").read_text())

    def test_source_drift_is_rejected_before_any_file_is_written(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            files = anchored_sources()
            files["gcc/config/mips/mips.c"] = "unexpected file start\n"
            write_tree(root, files)
            with self.assertRaisesRegex(ValueError, "assembly file start"):
                transformer.patch(root)
            self.assertEqual({name: (root / name).read_text() for name in files}, files)

    def test_a_second_application_is_refused_unchanged(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            write_tree(root, anchored_sources())
            transformer.patch(root)
            once = {p: p.read_text() for p in root.rglob("*") if p.is_file()}
            with self.assertRaisesRegex(ValueError, "already present"):
                transformer.patch(root)
            self.assertEqual({p: p.read_text() for p in root.rglob("*") if p.is_file()}, once)


class CompilerOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools"))
        if not all((compiler / name).is_file() for name in ("cpp", "cc1", "as")):
            if "RAC2_COMPILER_ROOT" in os.environ:
                raise RuntimeError("RAC2_COMPILER_ROOT must contain the newly built cpp, cc1 and as")
            raise unittest.SkipTest("EE compiler absent; tools/compiler/build.sh runs these checks after building it")
        cls.text, cls.c, cls.authored = {}, {}, {}
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            source = root / "input.c"
            source.write_bytes((ROOT / "tests/fixtures/hazard_labels.c").read_bytes())
            with (root / "input.i").open("w") as out:
                subprocess.run([str(compiler / "cpp"), str(source)], stdout=out, check=True)
            for label, options in (("default", []), ("enabled", ["-frac2-hazard-labels"]),
                                   ("disabled", ["-frac2-hazard-labels", "-fno-rac2-hazard-labels"])):
                asm, obj = root / (label + ".s"), root / (label + ".o")
                subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                                *options, str(root / "input.i"), "-o", str(asm)], check=True)
                subprocess.run([str(compiler / "as"), "-o", str(obj), str(asm)], check=True)
                cls.text[label], cls.c[label] = asm.read_text(), functions(obj)
            authored = (ROOT / "tests/fixtures/hazard_labels.s").read_text()
            for label, text in (("plain", authored), ("directive", DIRECTIVE + authored)):
                asm, obj = root / (label + "-authored.s"), root / (label + "-authored.o")
                asm.write_text(text)
                subprocess.run([str(compiler / "as"), "-o", str(obj), str(asm)], check=True)
                cls.authored[label] = functions(obj)

    def assert_label_moves_onto_the_nop(self, before, after, producer):
        """AFTER differs from BEFORE in one branch only: BEFORE reaches the instruction behind
        a hazard nop, AFTER that nop, which follows PRODUCER(word)."""
        self.assertEqual(len(before), len(after))
        changed = [i for i in range(len(before)) if before[i] != after[i]]
        self.assertEqual(len(changed), 1)
        branch = changed[0]
        old, new = branch_targets(before)[branch], branch_targets(after)[branch]
        self.assertEqual(new, old - 1)
        self.assertEqual(after[new], 0)
        self.assertTrue(producer(after[new - 1]))
        return old

    def test_option_writes_the_directive_only_when_enabled(self):
        self.assertNotIn("rac2_hazard_labels", self.text["default"])
        self.assertEqual(self.text["enabled"].count(DIRECTIVE), 1)
        self.assertEqual(self.text["enabled"].replace(DIRECTIVE, "", 1), self.text["default"])
        self.assertEqual(self.text["disabled"], self.text["default"])
        self.assertEqual(self.c["disabled"], self.c["default"])

    def test_forward_join_after_a_reload_lands_on_the_nop(self):
        mtc1 = lambda word: word >> 21 == 0x224          # COP1 MT
        old = self.assert_label_moves_onto_the_nop(self.c["default"]["clamp"],
                                                   self.c["enabled"]["clamp"], mtc1)
        self.assertEqual(self.c["enabled"]["clamp"][old] & 0xFFE0003F, SUB_S)
        self.assertEqual(len(self.c["enabled"]["clamp"]), 20)

    def test_c_controls_keep_their_bytes(self):
        for name in ("scale", "pick", "count"):
            with self.subTest(control=name):
                self.assertEqual(self.c["enabled"][name], self.c["default"][name])

    def test_forward_labels_land_on_the_nop(self):
        mtc1 = lambda word: word >> 21 == 0x224
        compare = lambda word: word >> 26 == 17 and (word >> 21) & 31 == 16 and (word & 0x30) == 0x30
        for name, producer in (("forward_transfer", mtc1), ("forward_compare", compare)):
            with self.subTest(case=name):
                self.assert_label_moves_onto_the_nop(self.authored["plain"][name],
                                                     self.authored["directive"][name], producer)

    def test_a_backward_label_stays_in_front_of_the_nop_too(self):
        mtc1 = lambda word: word >> 21 == 0x224
        self.assert_label_moves_onto_the_nop(self.authored["plain"]["backward_transfer"],
                                             self.authored["directive"]["backward_transfer"], mtc1)

    def test_assembler_controls_keep_their_bytes(self):
        for name in ("aligned_loop", "no_hazard", "forward_no_reader", "flushed_no_reader",
                     "transfer_from_fpu", "division", "backward_no_hazard"):
            with self.subTest(control=name):
                self.assertEqual(self.authored["directive"][name], self.authored["plain"][name])
        loop = self.authored["plain"]["aligned_loop"]
        self.assertEqual(loop[1], 0)                      # the alignment fill before the head
        self.assertEqual(branch_targets(loop)[3], 2)      # the head is behind it either way
        flushed = self.authored["plain"]["flushed_no_reader"]
        self.assertEqual(branch_targets(flushed)[0], 3)   # the beq right after the mtc1
        self.assertEqual(flushed[3] >> 26, 4)


if __name__ == "__main__":
    unittest.main()
