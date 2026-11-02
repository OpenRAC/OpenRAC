"""The opt-in -frac2-no-annulled-wide-stores: transformer anchors and EE compiler-output regressions.

With the option, reorg keeps 64- and 128-bit stores out of the annulled delay slots it fills
from a branch's target (tools/compiler/transformers/no_annulled_wide_stores.py). Ordinary CI
runs the transformer tests. The output tests need the newly built tools: they skip without
them, and RAC2_COMPILER_ROOT naming a directory without them is an error.
"""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("no_annulled_wide_stores",
    ROOT / "tools/compiler/transformers/no_annulled_wide_stores.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

OPTION = "rac2-no-annulled-wide-stores"
FUNCTIONS = ("thread_sq", "thread_sd", "steal_sd", "thread_sw", "skip_sd", "branch_slot_sd",
             "jump_slot_sq")
# the plain profile, and one without jump threading, where the loop of steal_sd keeps its
# branch to the skip branch and reorg steals that branch's slot
PROFILES = {"plain": [], "unthreaded": ["-fno-thread-jumps"]}
LIKELY = re.compile(r"^(beql|bnel|blezl|bgtzl|bltzl|bgezl|bc1fl|bc1tl)\s")
BRANCH = re.compile(r"^(beq|bne|blez|bgtz|bltz|bgez|bc1f|bc1t|b|j|jal|jr)l?\s")
WIDE = re.compile(r"^(sd|sq)\s")


NOREORDER, REORDER = ".set\tnoreorder", ".set\treorder"


def function_lines(asm: str, name: str) -> list[str]:
    """The function's instructions, labels and noreorder brackets, its local labels renumbered
    in order of appearance (a label added to an earlier function renumbers the later ones)."""
    body = re.search(r"(?ms)^" + name + r":\n(.*?)\.end\s+" + name, asm).group(1)
    labels = {}
    body = re.sub(r"\$L\d+", lambda m: labels.setdefault(m.group(), f"$L{len(labels)}"), body)
    lines = [line.strip() for line in body.splitlines()]
    return [line for line in lines if line and not line.startswith("#")
            and (not line.startswith(".") or line in (NOREORDER, REORDER))]


def slots(lines: list[str], kind: re.Pattern) -> list[tuple[str, str]]:
    """(branch, the instruction in its delay slot) for every branch of KIND whose slot cc1
    filled itself: cc1 writes such a branch and its slot between `.set noreorder' and
    `.set reorder'."""
    out, noreorder = [], False
    for i, line in enumerate(lines):
        if line in (NOREORDER, REORDER):
            noreorder = line == NOREORDER
        elif noreorder and kind.match(line):
            out.append((line, lines[i + 1]))
    return out


def anchored_sources() -> dict:
    return {"reorg.c": "before\n" + transformer.PREDICATE_OLD + "middle\n" + transformer.STEAL_OLD
                       + "middle\n" + transformer.THREAD_OLD + "after\n",
            "toplev.c": transformer.DEFINITION_OLD + transformer.OPTION_OLD,
            "flags.h": transformer.DECLARATION_OLD}


class TransformerTests(unittest.TestCase):
    def write(self, root: Path, files: dict) -> None:
        for name, contents in files.items():
            (root / name).write_text(contents)

    def test_each_insertion_point_is_used_once(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            self.write(root, anchored_sources())
            transformer.patch(root / "reorg.c")
            reorg = (root / "reorg.c").read_text()
            self.assertEqual(reorg.count("rac2_wide_store_p (insn)\n"), 1)
            self.assertEqual(reorg.count("flag_rac2_no_annulled_wide_stores"), 2)
            self.assertIn("\t         && ! (flag_rac2_no_annulled_wide_stores\n"
                          "\t\t       && rac2_wide_store_p (trial))))\n", reorg)
            self.assertIn("\t  else if (flag_rac2_no_annulled_wide_stores\n"
                          "\t\t   && rac2_wide_store_p (trial))\n\t    break;\n\t  else if (0\n", reorg)
            self.assertNotIn(transformer.STEAL_OLD, reorg)
            top = (root / "toplev.c").read_text()
            self.assertIn("int flag_rac2_no_annulled_wide_stores = 0;", top)
            self.assertIn('{"' + OPTION + '", &flag_rac2_no_annulled_wide_stores, 1,', top)
            self.assertLess(top.index(OPTION), top.index('"rac2-qcopy"'))
            self.assertIn("extern int flag_rac2_no_annulled_wide_stores;", (root / "flags.h").read_text())

    def test_source_drift_is_rejected_before_any_file_is_written(self):
        for name, label in (("reorg.c", "fill_slots_from_thread"), ("toplev.c", "option table"),
                            ("flags.h", "option declaration")):
            with self.subTest(drifted=name), tempfile.TemporaryDirectory() as d:
                root = Path(d)
                files = anchored_sources()
                if name == "reorg.c":
                    files[name] = files[name].replace(transformer.THREAD_OLD, "\t\tgoto winner;\n")
                elif name == "toplev.c":
                    files[name] = transformer.DEFINITION_OLD
                else:
                    files[name] = "unexpected declaration\n"
                self.write(root, files)
                with self.assertRaisesRegex(ValueError, label):
                    transformer.patch(root / "reorg.c")
                self.assertEqual({n: (root / n).read_text() for n in files}, files)

    def test_a_second_application_is_refused_unchanged(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            self.write(root, anchored_sources())
            transformer.patch(root / "reorg.c")
            once = {p.name: p.read_text() for p in root.iterdir()}
            with self.assertRaisesRegex(ValueError, "already present"):
                transformer.patch(root / "reorg.c")
            self.assertEqual({p.name: p.read_text() for p in root.iterdir()}, once)


class CompilerOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools"))
        if not all((compiler / name).is_file() for name in ("cpp", "cc1", "as")):
            if "RAC2_COMPILER_ROOT" in os.environ:
                raise RuntimeError("RAC2_COMPILER_ROOT must contain the newly built cpp, cc1 and as")
            raise unittest.SkipTest("EE compiler absent; tools/compiler/build.sh runs these checks after building it")
        cls.text, cls.lines = {}, {}
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            source = root / "input.c"
            source.write_bytes((ROOT / "tests/fixtures/no_annulled_wide_stores.c").read_bytes())
            with (root / "input.i").open("w") as out:
                try:
                    subprocess.run([str(compiler / "cpp"), str(source)], stdout=out, check=True)
                except OSError:
                    if "RAC2_COMPILER_ROOT" in os.environ:
                        raise
                    raise unittest.SkipTest("EE compiler not runnable on this host; tools/compiler/build.sh "
                                            "runs these checks in its build environment")
            for profile, extra in PROFILES.items():
                for mode, options in (("default", []), ("enabled", ["-f" + OPTION]),
                                      ("disabled", ["-f" + OPTION, "-fno-" + OPTION])):
                    label = profile + "-" + mode
                    asm = root / (label + ".s")
                    subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                                    *extra, *options, str(root / "input.i"), "-o", str(asm)], check=True)
                    subprocess.run([str(compiler / "as"), "-o", str(root / (label + ".o")), str(asm)],
                                   check=True)
                    cls.text[profile, mode] = asm.read_text()
                    cls.lines[profile, mode] = {name: function_lines(cls.text[profile, mode], name)
                                                for name in FUNCTIONS}

    def wide_annulled(self, profile: str, mode: str, name: str) -> list[tuple[str, str]]:
        return [pair for pair in slots(self.lines[profile, mode][name], LIKELY) if WIDE.match(pair[1])]

    def test_disabled_option_restores_default_output(self):
        for profile in PROFILES:
            with self.subTest(profile=profile):
                self.assertEqual(self.text[profile, "disabled"], self.text[profile, "default"])

    def test_default_puts_the_loop_top_store_in_an_annulled_slot(self):
        for name, store in (("thread_sq", "sq $0,0($sp)"), ("thread_sd", "sd\t$0,0($17)")):
            with self.subTest(case=name):
                self.assertEqual([slot for _, slot in self.wide_annulled("plain", "default", name)], [store])

    def test_option_keeps_the_store_under_the_loop_label(self):
        """The search of the loop's target thread stops at the store: no later insn of the loop
        is taken into the closing branch's slot either."""
        for name, store in (("thread_sq", "sq $0,0($sp)"), ("thread_sd", "sd\t$0,0($17)")):
            with self.subTest(case=name):
                lines = self.lines["plain", "enabled"][name]
                self.assertEqual(self.wide_annulled("plain", "enabled", name), [])
                head = lines.index(store) - 1
                self.assertRegex(lines[head], r"^\$L\d+:$")
                closing = [line for line in lines if re.match(r"^bnel?\s", line)]
                self.assertEqual(closing, ["bne\t$16,$0," + lines[head][:-1]])
                self.assertEqual(slots(lines, re.compile(r"^bne\s")), [])

    def test_option_refuses_the_stolen_store(self):
        """steal_sd's loop branch goes to the skip branch at the loop top; by default reorg
        copies that branch's annulled store into its own annulled slot."""
        skip = ("bnel", "sd\t$5,0($4)")
        default = self.wide_annulled("unthreaded", "default", "steal_sd")
        enabled = self.wide_annulled("unthreaded", "enabled", "steal_sd")
        self.assertEqual([(b.split()[0], s) for b, s in default], [skip, skip])
        self.assertEqual([(b.split()[0], s) for b, s in enabled], [skip])
        self.assertEqual(len([line for line in self.lines["unthreaded", "enabled"]["steal_sd"]
                              if re.match(r"^bne\s", line)]), 1)

    def test_the_thread_path_of_the_same_loop(self):
        self.assertEqual(len(self.wide_annulled("plain", "default", "steal_sd")), 1)
        self.assertEqual(self.wide_annulled("plain", "enabled", "steal_sd"), [])

    def test_controls_keep_their_output(self):
        for profile in PROFILES:
            for name in ("thread_sw", "skip_sd", "branch_slot_sd", "jump_slot_sq"):
                with self.subTest(profile=profile, control=name):
                    self.assertEqual(self.lines[profile, "enabled"][name], self.lines[profile, "default"][name])

    def test_controls_hold_what_they_control(self):
        lines = self.lines["plain", "enabled"]
        self.assertEqual([s for _, s in slots(lines["thread_sw"], LIKELY)], ["sw\t$0,0($17)"])
        self.assertEqual([(b.split()[0], s) for b, s in slots(lines["skip_sd"], LIKELY)],
                         [("beql", "sd\t$5,0($4)")])
        ordinary = lambda name: [(b.split()[0], s) for b, s in slots(lines[name], BRANCH)
                                 if not LIKELY.match(b) and WIDE.match(s)]
        self.assertEqual(ordinary("branch_slot_sd"), [("beq", "sd\t$5,0($4)")])
        self.assertIn(("b", "sq $0,0($sp)"), ordinary("jump_slot_sq"))


if __name__ == "__main__":
    unittest.main()
