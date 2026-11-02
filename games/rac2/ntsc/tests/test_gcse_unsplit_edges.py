"""The opt-in -frac2-gcse-unsplit-edges: transformer drift and EE compiler-output regressions."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("gcse_unsplit_edges",
    ROOT / "tools/compiler/transformers/gcse_unsplit_edges.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

OPTION = "rac2-gcse-unsplit-edges"
FUNCTIONS = ("hoist_split", "diamond", "invariant")
SPLIT_BLOCK = r"\$L\d+:\n\tlui\t\$\d+,%hi\(table\)"       # a label, then only the high half


def function_text(asm: str, name: str) -> str:
    """The function's assembly, its local labels renumbered in order of appearance (a label
    added to an earlier function renumbers the later ones)."""
    body = re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, asm).group()
    labels = {}
    return re.sub(r"\$L\d+", lambda m: labels.setdefault(m.group(), f"$L{len(labels)}"), body)


def first_branch(text: str) -> int:
    return re.search(r"\n\tb[a-z]*\t", text).start()


class TransformerTests(unittest.TestCase):
    def sources(self, root: Path, **override) -> dict:
        files = {"gcse.c": "before\n" + transformer.CALL_OLD + "after\n",
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
            transformer.patch(root / "gcse.c")
            gcse = (root / "gcse.c").read_text()
            self.assertIn("! flag_rac2_gcse_unsplit_edges) != 0)", gcse)
            self.assertNotIn(transformer.CALL_OLD, gcse)
            top = (root / "toplev.c").read_text()
            self.assertIn("int flag_rac2_gcse_unsplit_edges = 0;", top)
            self.assertIn('{"' + OPTION + '", &flag_rac2_gcse_unsplit_edges, 1,', top)
            self.assertIn("extern int flag_rac2_gcse_unsplit_edges;", (root / "flags.h").read_text())
            with self.assertRaisesRegex(ValueError, "gcse_main's split"):
                transformer.patch(root / "gcse.c")

    def test_source_drift_is_rejected_before_any_file_is_written(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            files = self.sources(root, **{"flags.h": "unexpected declaration"})
            with self.assertRaisesRegex(ValueError, "option declaration"):
                transformer.patch(root / "gcse.c")
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
            source.write_bytes((ROOT / "tests/fixtures/gcse_unsplit_edges.c").read_bytes())
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

    def test_default_splits_the_edge_into_the_jump_only_arm(self):
        default = self.outputs["default"]["hoist_split"]
        self.assertRegex(default, SPLIT_BLOCK)
        self.assertGreater(default.index("%hi(table)"), first_branch(default))

    def test_option_computes_the_high_half_in_the_entry_block(self):
        enabled = self.outputs["enabled"]["hoist_split"]
        self.assertNotRegex(enabled, SPLIT_BLOCK)
        self.assertLess(enabled.index("%hi(table)"), first_branch(enabled))
        self.assertEqual(enabled.count("%hi(table)"), 1)

    def test_controls_keep_ordinary_behaviour(self):
        for name in ("diamond", "invariant"):
            with self.subTest(control=name):
                self.assertEqual(self.outputs["default"][name], self.outputs["enabled"][name])


if __name__ == "__main__":
    unittest.main()
