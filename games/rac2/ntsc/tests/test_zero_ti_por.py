"""-frac2-zero-ti-por: the transformer's anchors, the stack clears it keeps and the stores it still folds."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / f"tools/compiler/transformers/{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


transformer = load("zero_ti_por")
fold = load("fold_zero_ti_store")

NAMES = ("stack", "stack_loop", "pointer", "global", "mixed", "stack_offset", "pointer_offset", "several",
         "value", "live", "loop")


def tree(root: Path, mips: str, toplev: str, flags: str) -> dict:
    files = {"config/mips/mips.c": mips, "toplev.c": toplev, "flags.h": flags}
    (root / "config/mips").mkdir(parents=True)
    for name, contents in files.items():
        (root / name).write_text(contents)
    return files


class TransformerTests(unittest.TestCase):
    MIPS = ("void\nrac2_fold_zero_ti_store (first)\n{\n" + transformer.PASS_OLD
            + "      live = rac2_reg_live_after_store_p (insn, reg);\n")
    TOPLEV = (transformer.DEFINITION_OLD + "int flag_rac2_qcopy = 0;\n"
              + '  {"rac2-zero-ti-barrier", &flag_rac2_zero_ti_barrier, 1,\n' + transformer.OPTION_OLD)
    FLAGS = transformer.DECLARATION_OLD

    def test_anchors_are_the_texts_the_earlier_transformers_write(self):
        # The fold pass as fold_zero_ti_store.py inserts it, and the clear barrier's option lines.
        self.assertEqual(fold.FUNC.count(transformer.PASS_OLD), 1)
        barrier = (ROOT / "tools/compiler/transformers/zero_ti_store_barrier.py").read_text()
        for line in (transformer.DEFINITION_OLD, transformer.DECLARATION_OLD,
                     transformer.OPTION_OLD.splitlines()[0]):
            self.assertIn(line.strip(), barrier)

    def test_patch_skips_only_stack_pointer_stores_and_applies_once(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            tree(root, self.MIPS, self.TOPLEV, self.FLAGS)
            transformer.patch(root)
            mips = (root / "config/mips/mips.c").read_text()
            # After the bare-register test and before the zero register is examined: a store
            # addressed by $sp alone is skipped, any other register address still folds.
            guard = mips.split("!= REG)\n        continue;\n", 1)[1].split("reg = SET_SRC (set);", 1)[0]
            self.assertRegex(guard, r"if \(flag_rac2_zero_ti_por\s*&& REGNO \(XEXP \(SET_DEST \(set\), 0\)\)"
                                    r" == STACK_POINTER_REGNUM\)\s*continue;\s*$")
            self.assertEqual(mips.count("flag_rac2_zero_ti_por"), 1)
            top = (root / "toplev.c").read_text()
            self.assertIn("int flag_rac2_zero_ti_por = 0;", top)
            self.assertIn('{"rac2-zero-ti-por", &flag_rac2_zero_ti_por, 1,', top)
            self.assertLess(top.index('"rac2-zero-ti-barrier"'), top.index('"rac2-zero-ti-por"'))
            self.assertIn("extern int flag_rac2_zero_ti_por;", (root / "flags.h").read_text())
            with self.assertRaisesRegex(ValueError, "already present"):
                transformer.patch(root)

    def test_source_drift_is_rejected_before_any_file_is_written(self):
        for broken, label in (("config/mips/mips.c", "rac2_fold_zero_ti_store"),
                              ("toplev.c", "option table"), ("flags.h", "option declaration")):
            with self.subTest(drift=broken), tempfile.TemporaryDirectory() as d:
                root = Path(d)
                texts = {"config/mips/mips.c": self.MIPS, "toplev.c": self.TOPLEV, "flags.h": self.FLAGS}
                texts[broken] = {"config/mips/mips.c": "void\nrac2_fold_zero_ti_store (first)\n",
                                 "toplev.c": transformer.DEFINITION_OLD,
                                 "flags.h": "unexpected declaration\n"}[broken]
                files = tree(root, texts["config/mips/mips.c"], texts["toplev.c"], texts["flags.h"])
                with self.assertRaisesRegex(ValueError, label):
                    transformer.patch(root)
                self.assertEqual({name: (root / name).read_text() for name in files}, files)


def numbered(body: str) -> str:
    """One function's assembly with its local labels numbered from 1 in order of appearance: an
    earlier function's delay-slot filling may number them differently, which no object keeps."""
    labels = {}
    return re.sub(r"\$L\d+", lambda m: labels.setdefault(m.group(), f"$L{len(labels) + 1}"), body)


def instructions(body: str) -> list[str]:
    """The instruction lines of one function's assembly, without directives, labels or comments."""
    lines = (line.split("#")[0].strip() for line in body.splitlines())
    return [line for line in lines if line and not line.startswith(".") and not line.endswith(":")]


class CompilerOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools"))
        if not all((compiler / name).is_file() for name in ("cpp", "cc1", "as")):
            if "RAC2_COMPILER_ROOT" in os.environ:
                raise RuntimeError("RAC2_COMPILER_ROOT must contain the newly built cpp, cc1 and as")
            raise unittest.SkipTest("EE compiler absent; tools/compiler/build.sh runs these checks after building it")
        cls.outputs = {}
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            source = root / "input.c"
            source.write_bytes((ROOT / "tests/fixtures/zero_ti_por.c").read_bytes())
            with (root / "input.i").open("w") as out:
                subprocess.run([str(compiler / "cpp"), str(source)], stdout=out, check=True)
            for label, options in (("default", []), ("enabled", ["-frac2-zero-ti-por"]),
                                   ("disabled", ["-frac2-zero-ti-por", "-fno-rac2-zero-ti-por"])):
                asm = root / (label + ".s")
                subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                                *options, str(root / "input.i"), "-o", str(asm)], check=True)
                subprocess.run([str(compiler / "as"), "-o", str(root / (label + ".o")), str(asm)], check=True)
                text = asm.read_text()
                cls.outputs[label] = {name: numbered(re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name,
                                                               text).group())
                                      for name in NAMES}

    def code(self, profile, name):
        return instructions(self.outputs[profile][name])

    def test_disabled_option_restores_default_for_every_control(self):
        self.assertEqual(self.outputs["default"], self.outputs["disabled"])

    def test_stack_clears_fold_by_default_and_keep_their_por_with_the_option(self):
        for name in ("stack", "stack_loop"):
            with self.subTest(control=name):
                default, enabled = "\n".join(self.code("default", name)), "\n".join(self.code("enabled", name))
                self.assertRegex(default, r"sq\s+\$0,0\(\$sp\)")
                self.assertNotIn("por", default)
                zero = re.findall(r"por\s+(\$\d+),\$0,\$0", enabled)
                self.assertEqual(len(zero), 1)
                self.assertRegex(enabled, r"sq\s+" + re.escape(zero[0]) + r",0\(\$sp\)")
                self.assertNotRegex(enabled, r"sq\s+\$0,")

    def test_the_option_changes_only_the_stack_clear_and_its_materialisation(self):
        # Everything but the zero's `por`, the stores and the scheduler's nops is the same code.
        def rest(profile, name):
            return sorted(line for line in self.code(profile, name)
                          if not line.startswith(("por", "sq", "nop")))
        for name in ("stack", "stack_loop", "mixed"):
            with self.subTest(control=name):
                self.assertEqual(rest("default", name), rest("enabled", name))

    def test_clears_through_other_registers_still_fold_with_the_option(self):
        for name, base in (("pointer", r"\$4"), ("global", r"\$2"), ("loop", r"\$4"), ("value", r"\$4")):
            with self.subTest(control=name):
                self.assertEqual(self.outputs["default"][name], self.outputs["enabled"][name])
                self.assertRegex(self.outputs["enabled"][name], r"sq\s+\$0,0\(" + base + r"\)")
                self.assertNotRegex(self.outputs["enabled"][name], r"\bpor\s")
        self.assertRegex(self.outputs["enabled"]["global"], r"%lo\(table\)")
        self.assertIn("move\t$2,$0", self.code("enabled", "value"))

    def test_a_mixed_function_keeps_only_its_stack_clear(self):
        default, enabled = "\n".join(self.code("default", "mixed")), "\n".join(self.code("enabled", "mixed"))
        self.assertEqual(len(re.findall(r"sq\s+\$0,0\(", default)), 2)
        self.assertNotIn("por", default)
        zero = re.findall(r"por\s+(\$\d+),\$0,\$0", enabled)
        self.assertEqual(len(zero), 1)
        self.assertRegex(enabled, r"sq\s+" + re.escape(zero[0]) + r",0\(\$sp\)")
        self.assertEqual(re.findall(r"sq\s+\$0,0\((\$\w+)\)", enabled), ["$2"])

    def test_offsets_shared_and_live_zeros_are_unchanged(self):
        for name in ("stack_offset", "pointer_offset", "several", "live"):
            with self.subTest(control=name):
                self.assertEqual(self.outputs["default"][name], self.outputs["enabled"][name])
                zero = re.findall(r"por\s+(\$\d+),\$0,\$0", self.outputs["enabled"][name])
                self.assertEqual(len(zero), 1)
                self.assertNotRegex(self.outputs["enabled"][name], r"sq\s+\$0,")
        for name, store in (("stack_offset", r",16\(\$sp\)"), ("pointer_offset", r",16\(\$4\)"),
                            ("live", r",0\(\$4\)")):
            with self.subTest(control=name):
                self.assertRegex(self.outputs["enabled"][name], r"sq\s+\$\d+" + store)
        several = self.outputs["enabled"]["several"]
        zero = re.search(r"por\s+(\$\d+),\$0,\$0", several).group(1)
        self.assertEqual(len(re.findall(r"sq\s+" + re.escape(zero) + r",", several)), 3)



if __name__ == "__main__":
    unittest.main()
