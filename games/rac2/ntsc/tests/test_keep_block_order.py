"""The opt-in -frac2-keep-block-order: transformer drift and EE compiler-output regressions."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("keep_block_order",
    ROOT / "tools/compiler/transformers/keep_block_order.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

OPTION = "rac2-keep-block-order"
FUNCTIONS = ("default_last", "default_first", "chain", "loop")
DISPATCH_END = r"\tbeq\t\$4,\$2,\$L\d+\n\tb\t\$L\d+\n"     # the last test, then `b default`


def function_text(asm: str, name: str) -> str:
    """The function's assembly, its local labels renumbered in order of appearance (a label
    added to an earlier function renumbers the later ones)."""
    body = re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, asm).group()
    labels = {}
    return re.sub(r"\$L\d+", lambda m: labels.setdefault(m.group(), f"$L{len(labels)}"), body)


def body_order(text: str) -> list:
    """Where the three case bodies start: the 8 and 9 arguments, and the default's call."""
    return [text.index(s) for s in ("li\t$4,8", "li\t$4,9", "jal\tvalue")]


class TransformerTests(unittest.TestCase):
    def sources(self, root: Path, **override) -> dict:
        files = {"flow.c": "before\n" + transformer.PASS_OLD + "\nafter\n",
                 "toplev.c": transformer.DEFINITION_OLD + transformer.OPTION_OLD,
                 "flags.h": transformer.DECLARATION_OLD}
        files.update(override)
        for name, contents in files.items():
            (root / name).write_text(contents)
        return files

    def test_each_insertion_point_is_used_once(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            self.sources(root)
            transformer.patch(root / "flow.c")
            self.assertIn("  if (flag_rac2_keep_block_order)\n    return;\n", (root / "flow.c").read_text())
            top = (root / "toplev.c").read_text()
            self.assertIn("int flag_rac2_keep_block_order = 0;", top)
            self.assertIn('{"' + OPTION + '", &flag_rac2_keep_block_order, 1,', top)
            self.assertIn("extern int flag_rac2_keep_block_order;", (root / "flags.h").read_text())
            with self.assertRaisesRegex(ValueError, "cygnus_merge_blocks"):
                transformer.patch(root / "flow.c")

    def test_source_drift_is_rejected_before_any_file_is_written(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            files = self.sources(root, **{"flags.h": "unexpected declaration"})
            with self.assertRaisesRegex(ValueError, "option declaration"):
                transformer.patch(root / "flow.c")
            self.assertEqual({name: (root / name).read_text() for name in files}, files)


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
            source.write_bytes((ROOT / "tests/fixtures/keep_block_order.c").read_bytes())
            with (root / "input.i").open("w") as out:
                try:
                    subprocess.run([str(compiler / "cpp"), str(source)], stdout=out, check=True)
                except OSError:
                    if "RAC2_COMPILER_ROOT" in os.environ:
                        raise
                    raise unittest.SkipTest("EE compiler not runnable on this host; tools/compiler/build.sh "
                                            "runs these checks in its build environment")
            for label, options in (("default", []), ("enabled", ["-f" + OPTION]),
                                   ("disabled", ["-f" + OPTION, "-fno-" + OPTION])):
                asm = root / (label + ".s")
                subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                                *options, str(root / "input.i"), "-o", str(asm)], check=True)
                subprocess.run([str(compiler / "as"), "-o", str(root / (label + ".o")), str(asm)], check=True)
                text = asm.read_text()
                cls.outputs[label] = {name: function_text(text, name) for name in FUNCTIONS}

    def test_disabled_option_restores_default_for_every_function(self):
        self.assertEqual(self.outputs["default"], self.outputs["disabled"])

    def test_default_moves_the_default_body_behind_the_dispatch(self):
        default = self.outputs["default"]["default_last"]
        eight, nine, default_body = body_order(default)
        self.assertLess(default_body, eight)
        self.assertNotRegex(default, DISPATCH_END)

    def test_option_keeps_b_default_and_the_default_body_last(self):
        enabled = self.outputs["enabled"]["default_last"]
        eight, nine, default_body = body_order(enabled)
        self.assertTrue(eight < nine < default_body)
        self.assertRegex(enabled, DISPATCH_END)

    def test_controls_keep_ordinary_behaviour(self):
        for name in ("default_first", "chain", "loop"):
            with self.subTest(control=name):
                self.assertEqual(self.outputs["default"][name], self.outputs["enabled"][name])


if __name__ == "__main__":
    unittest.main()
