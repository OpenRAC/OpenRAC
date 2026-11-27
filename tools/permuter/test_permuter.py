# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""tools/permuter on synthetic code and files only, never a game: python3 -m unittest discover -s tools"""

import contextlib
import io
import json
import struct
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from permuter import accept, cli, elf, engine, profile, runner as queue, score, upstream

ADDIU_SP = 0x27BDFFF0
JR_RA = 0x03E00008


def lui(rt, imm):
    return 0x3C000000 | rt << 16 | imm


def lw(rt, base, imm):
    return 0x8C000000 | base << 21 | rt << 16 | imm & 0xFFFF


def jal(target):
    return 0x0C000000 | target >> 2 & 0x3FFFFFF


def beq(rs, rt, off):
    return 0x10000000 | rs << 21 | rt << 16 | off & 0xFFFF


def asm(*lines):
    """splat lines of (word, text), the word written as stored (le-bytes)."""
    out = ["nonmatching func_00100000, 0x20", "glabel func_00100000"]
    for i, (w, text) in enumerate(lines):
        stored = struct.pack("<I", w).hex().upper()
        out.append(f"    /* {i * 4:X} {0x100000 + i * 4:08X} {stored} */  {text}")
    out += ["endlabel func_00100000", "glabel func_00100020", "    /* 20 00100020 0800E003 */ jr $ra"]
    return "\n".join(out)


def obj(words, relocs):
    """A relocatable ELF32 holding func_00100000 = WORDS in .text, RELOCS {index: (type, symbol)}."""
    text = b"".join(struct.pack("<I", w) for w in words)
    names = sorted({s for _, s in relocs.values()})
    strtab = b"\0func_00100000\0" + b"".join(n.encode() + b"\0" for n in names)
    offsets = {n: strtab.index(b"\0" + n.encode() + b"\0") + 1 for n in names}
    syms = struct.pack("<IIIBBH", 0, 0, 0, 0, 0, 0) + struct.pack("<IIIBBH", 1, 0, len(text), 0x12, 0, 1)
    index = {n: 2 + i for i, n in enumerate(names)}
    syms += b"".join(struct.pack("<IIIBBH", offsets[n], 0, 0, 0x10, 0, 0) for n in names)
    rel = b"".join(struct.pack("<II", i * 4, index[s] << 8 | t) for i, (t, s) in sorted(relocs.items()))
    shstr = b"\0.text\0.symtab\0.strtab\0.rel.text\0.shstrtab\0"
    bodies = [text, syms, strtab, rel, shstr]
    at, offs = 52, []
    for b in bodies:
        offs.append(at)
        at += len(b)
    shoff = at
    sh = [struct.pack("<10I", *[0] * 10),
          struct.pack("<10I", 1, 1, 6, 0, offs[0], len(text), 0, 0, 4, 0),
          struct.pack("<10I", 7, 2, 0, 0, offs[1], len(syms), 3, 2, 4, 16),
          struct.pack("<10I", 15, 3, 0, 0, offs[2], len(strtab), 0, 0, 1, 0),
          struct.pack("<10I", 23, 9, 0, 0, offs[3], len(rel), 2, 1, 4, 8),
          struct.pack("<10I", 33, 3, 0, 0, offs[4], len(shstr), 0, 0, 1, 0)]
    header = b"\x7fELF\x01\x01\x01" + b"\0" * 9 + struct.pack("<HHIIIIIHHHHHH", 1, 8, 1, 0, 0, shoff, 0, 52,
                                                             0, 0, 40, len(sh), 5)
    return header + b"".join(bodies) + b"".join(sh)


class ScoreTest(unittest.TestCase):
    RETAIL = asm((lui(2, 0x16), "lui $v0, %hi(D_00163E74)"), (lw(4, 2, 0x3EF4), "lw $a0, %lo(D_00163E74)($v0)"),
                 (jal(0x11A1B0), "jal func_0011A1B0"), (0, "nop"), (JR_RA, "jr $ra"), (0, "nop"), (0, "nop"))

    def retail(self):
        return score.parse_asm(self.RETAIL, "func_00100000")

    def candidate(self, words, relocs):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "c.o"
            path.write_bytes(obj(words, relocs))
            return score.from_object(path, "func_00100000")

    def test_parse_reads_words_masks_and_stops_at_the_next_function(self):
        f = self.retail()
        self.assertEqual(f.words[:3], [lui(2, 0x16), lw(4, 2, 0x3EF4), jal(0x11A1B0)])
        self.assertEqual(len(f.words), 6, "the padding nop after the delay slot is trimmed")
        self.assertEqual(f.masks[:3], [0xFFFF, 0xFFFF, 0x3FFFFFF])
        self.assertEqual(f.call(2), "func_0011A1B0")

    def test_word_order_word(self):
        text = "glabel func_00100000\n/* 11A1C8 3C020016 */ lui $v0, 0x16\n"
        self.assertEqual(score.parse_asm(text, "func_00100000", "word").words, [lui(2, 0x16)])

    def test_relocated_fields_match_and_a_wrong_callee_does_not(self):
        words = [lui(2, 0), lw(4, 2, 0), jal(0), 0, JR_RA, 0]
        relocs = {0: (5, "D_00163E74"), 1: (6, "D_00163E74"), 2: (4, "func_0011A1B0")}
        self.assertEqual(score.diffs(self.candidate(words, relocs), self.retail()), 0)
        relocs[2] = (4, "func_0011A1B8")
        self.assertEqual(score.diffs(self.candidate(words, relocs), self.retail()), 1)

    def test_an_unrelocated_offset_counts_and_a_missing_instruction_counts(self):
        retail = score.Function([ADDIU_SP, lw(4, 2, 0x3EF4), JR_RA, 0], [0] * 4)
        self.assertEqual(score.diffs(score.Function([ADDIU_SP, lw(4, 2, 0x10), JR_RA, 0], [0] * 4), retail), 1)
        self.assertEqual(score.diffs(score.Function([ADDIU_SP, JR_RA, 0], [0] * 3), retail), 1)

    def test_branches_compare_through_the_alignment(self):
        retail = score.Function([beq(2, 0, 1), 0, ADDIU_SP, JR_RA, 0], [0] * 5)
        same = score.Function([beq(2, 0, 1), 0, ADDIU_SP, JR_RA, 0], [0] * 5)
        other = score.Function([beq(2, 0, 2), 0, ADDIU_SP, JR_RA, 0], [0] * 5)
        self.assertEqual(score.diffs(same, retail), 0)
        self.assertEqual(score.diffs(other, retail), 1)


class ElfTest(unittest.TestCase):
    def test_not_elf(self):
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / "x").write_bytes(b"nope")
            with self.assertRaises(ValueError):
                elf.read(Path(d) / "x")

    def test_function_and_relocations(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "c.o"
            path.write_bytes(obj([ADDIU_SP, jal(0), 0], {1: (4, "func_00200000")}))
            code, relocs = elf.read(path).function("func_00100000")
        self.assertEqual(len(code), 12)
        self.assertEqual(relocs, {4: (4, "func_00200000")})


class ProfileTest(unittest.TestCase):
    SOURCE = ('#include "common.h"\n\nINCLUDE_ASM("asm/game", func_00100000);\n\n'
              'static int helper(int x) { return x; }\n\nint func_00100008(int a) {\n    if (a) { return helper(a); }\n'
              '    return 0;\n}\n')

    def test_definition_and_stub(self):
        start, end = profile.definition(self.SOURCE, "func_00100008")
        self.assertTrue(self.SOURCE[start:end].startswith("int func_00100008(int a) {"))
        self.assertTrue(self.SOURCE[start:end].endswith("}"))
        self.assertIsNone(profile.definition("extern int func_00100008(int);\n", "func_00100008"))
        start, end = profile.stub(self.SOURCE, "func_00100000")
        self.assertEqual(self.SOURCE[start:end], 'INCLUDE_ASM("asm/game", func_00100000);')

    def test_split_draft(self):
        decls, fn = profile.split_draft("extern int g;\nvoid func_00100000(void) { g = 1; }\n", "func_00100000")
        self.assertEqual(decls, "extern int g;\n")
        self.assertEqual(fn, "void func_00100000(void) { g = 1; }")
        with self.assertRaises(ValueError):
            profile.split_draft("int x;\n", "func_00100000")

    def test_strip_top_level_asm_keeps_asm_inside_functions(self):
        text = '__asm__(".extern D_1, 4");\nvoid f(void) { __asm__("" : "+r"(x)); }\n'
        self.assertEqual(profile.strip_top_level_asm(text).lstrip(), 'void f(void) { __asm__("" : "+r"(x)); }\n')
        self.assertEqual(profile.strip_top_level_asm('extern int g(int) __asm__("h");\n'), "extern int g(int) ;\n")

    def test_every_profile_loads_and_names_its_commands(self):
        for name in profile.names():
            p = profile.load(name)
            self.assertTrue(p.compile and p.exact and p.sources and p.asm, name)
            self.assertIn(p.word_order, ("le-bytes", "word"), name)
            self.assertIn("{func}", p.exact, name)

    def test_expand_and_rename(self):
        p = profile.load("rac2/ntsc")
        p.root = Path("/x")
        self.assertEqual(profile.retail_name(p, "func_L00_002AE070"), "f_002AE070")
        self.assertEqual(p.expand(["cc", "{flags}", "{checkout}/{func}"], {"flags": "-O2 -G8", "func": "f"}),
                         ["cc", "-O2", "-G8", "/x/f"])

    def test_rac5_profile_is_isolated_and_bounded(self):
        p = profile.load("rac5/ntsc")
        self.assertEqual((p.max_candidates, p.candidate_ceiling), (3, 3))
        self.assertTrue(p.keep_work)
        self.assertFalse(p.check_in_place)
        self.assertFalse(p.asm_passes)
        self.assertEqual({k: v for k, v in p.weights.items() if v}, {"perm_reverse_cond": 1})
        self.assertIn("{RAC5_BACKEND}", p.compile[0])
        self.assertIn("{RAC5_MATCH_RUNTIME}", p.compile[0])
        self.assertNotIn("try_body", repr(p))

    def test_other_profiles_keep_their_unbounded_defaults(self):
        p = profile.load("rac4/ntsc")
        self.assertEqual((p.max_candidates, p.candidate_ceiling), (0, 0))
        self.assertFalse(p.keep_work)
        self.assertTrue(p.asm_passes)

    def test_direct_preparation_inherits_profile_limits(self):
        p = profile.load("rac5/ntsc")
        p.flags = None
        p.weights["perm_reg_fence"] = 15
        p.weights["perm_sched_barrier"] = 10
        with tempfile.TemporaryDirectory() as d:
            source, retail = Path(d) / "f.c", Path(d) / "f.s"
            source.write_text("int f(void) { return 1; }\n")
            retail.write_text("glabel f\n/* 00100000 03E00008 */ jr $ra\n/* 00100004 00000000 */ nop\n")
            modules = SimpleNamespace(strip_other_fns=SimpleNamespace(strip_other_fns=lambda *a: ""))
            with mock.patch.object(engine.upstream, "load", return_value=modules), \
                    mock.patch.object(profile, "base_source", return_value=(source.read_text(), "")), \
                    mock.patch.object(profile, "find_source", return_value=source), \
                    mock.patch.object(profile, "find_asm", return_value=retail), \
                    mock.patch.object(engine.upstream, "has_asm_passes") as asm_passes:
                cfg = engine.prepare(p, "f", None, Path(d), asm_passes=True)
            self.assertEqual((cfg["max_candidates"], cfg["candidate_ceiling"]), (3, 3))
            self.assertTrue(cfg["keep_work"])
            asm_passes.assert_not_called()
            self.assertNotIn("perm_reg_fence", cfg["weights"])
            self.assertNotIn("perm_sched_barrier", cfg["weights"])

    def test_cli_ceiling_is_checked_before_backend_preparation(self):
        stderr = io.StringIO()
        with mock.patch.object(cli.upstream, "checkout") as checkout, \
                mock.patch.object(cli.engine, "run_start") as start, \
                mock.patch.object(cli.engine, "prepare") as prepare, contextlib.redirect_stderr(stderr):
            result = cli.main(["run", "rac5/ntsc", "unused_function", "-j1",
                               "--max-candidates", "4", "--no-asm-passes"])
        self.assertNotEqual(result, 0)
        self.assertIn("profile ceiling 3", stderr.getvalue())
        checkout.assert_not_called()
        start.assert_not_called()
        prepare.assert_not_called()

    def test_cli_reports_a_constructor_only_candidate(self):
        p = SimpleNamespace(name="synthetic")
        cfg = {"flags": ""}
        state = {"iterations": 0, "base_candidates": 1, "status": "matched", "base_score": 0,
                 "best_score": 0, "elapsed": 1.0}
        job = SimpleNamespace(jobs=1, state={"asm_passes": False},
                              thread=SimpleNamespace(is_alive=lambda: False), snapshot=lambda: state)
        job.start = lambda: job
        args = SimpleNamespace(func="f", out="unused", j=1, seconds=1)
        stdout = io.StringIO()
        with mock.patch.object(cli, "_prepare", return_value=(p, cfg)), \
                mock.patch.object(cli.engine, "Job", return_value=job), contextlib.redirect_stdout(stdout):
            self.assertEqual(cli.cmd_run(args), 0)
        self.assertIn("total candidates including bases: 1", stdout.getvalue())


class CandidateLimitTest(unittest.TestCase):
    def test_parallel_workers_cannot_multiply_a_total_limit(self):
        with self.assertRaisesRegex(ValueError, "one worker"):
            engine.Job({"max_candidates": 3}, Path("unused"), jobs=2)

    def test_direct_jobs_respect_a_profile_ceiling(self):
        for maximum in (0, 4):
            with self.subTest(maximum=maximum), self.assertRaisesRegex(ValueError, "profile ceiling 3"):
                engine.Job({"max_candidates": maximum, "candidate_ceiling": 3}, Path("unused"), jobs=1)

    def test_result_counts_each_workers_constructor_base(self):
        cfg = {"profile": "synthetic", "func": "f", "flags": "", "weights": {}}
        with tempfile.TemporaryDirectory() as d:
            job = engine.Job(cfg, Path(d), jobs=2)
            messages = iter([("base", 0, 5, "int f(void) { return 1; }"), ("base", 1, 5, None),
                             ("tick", 0, [(None, 5)]), ("done", 0), ("done", 1)])
            job._q = SimpleNamespace(get=lambda timeout: next(messages))
            job._collect()
            result = json.loads((Path(d) / "result.json").read_text())
        self.assertEqual(result["iterations"], 1)
        self.assertEqual(result["candidates_including_base"], 3)

    def test_worker_counts_the_upstream_constructor_base(self):
        # The real upstream constructor compiles once before seed_iterator.
        # A finite fake compiler makes that protocol observable without a disc,
        # proprietary compiler or an installed decomp-permuter dependency.
        for maximum, expected in ((1, 1), (3, 3), (0, 5)):
            with self.subTest(maximum=maximum), tempfile.TemporaryDirectory() as d:
                compiled, messages = [], []

                class Compiler:
                    last_error = ""

                    def __init__(self, cfg, tmp):
                        pass

                    def compile(self, source):
                        compiled.append(source)
                        return "object.o"

                class Randomizer:
                    def randomize(self, ast, fn_name):
                        pass

                class Permuter:
                    def __init__(self, directory, func, compiler, scorer, filename, source, **options):
                        self.compiler = compiler
                        compiler.compile("base")
                        self.base_score, self.base_source = 5, source

                    def seed_iterator(self):
                        return iter(range(4))

                    def try_eval_candidate(self, seed):
                        self.compiler.compile(f"candidate-{seed}")
                        return SimpleNamespace(score=5)

                    def should_output(self, result):
                        return False

                modules = SimpleNamespace(
                    helpers=SimpleNamespace(get_default_randomization_weights=lambda _: {}),
                    randomizer=SimpleNamespace(Randomizer=Randomizer),
                    permuter=SimpleNamespace(Permuter=Permuter, EvalError=type("EvalError", (), {})))
                cfg = {"permuter": "unused", "func": "f", "base": "int f(void) { return 1; }",
                       "weights": {}, "max_candidates": maximum, "keep_work": True}
                with mock.patch.object(engine.upstream, "load", return_value=modules), \
                        mock.patch.object(engine.upstream, "cache_dir", return_value=Path(d)), \
                        mock.patch.object(engine, "Candidate", Compiler):
                    engine._worker(cfg, 0, SimpleNamespace(put=messages.append),
                                   SimpleNamespace(is_set=lambda: False), 1)
                self.assertEqual(compiled[0], "base")
                self.assertEqual(len(compiled), expected)
                self.assertFalse(any(m[0] == "fatal" for m in messages), messages)
                self.assertEqual(messages[-1], ("done", 0))
                self.assertTrue(list((Path(d) / "work").glob("w0-*")), "keep_work retains compiler evidence")


class QueueTest(unittest.TestCase):
    def test_verdicts(self):
        self.assertEqual(queue.verdict({"status": "matched", "base_score": 3, "best_score": 0, "best": {"artefacts": []}}), "WIN")
        self.assertEqual(queue.verdict({"status": "matched", "base_score": 3, "best_score": 0,
                                        "best": {"artefacts": ["dead store: x"]}}), "artefact")
        self.assertEqual(queue.verdict({"status": "time limit", "base_score": 3, "best_score": 2, "outputs": []}), "better")
        self.assertEqual(queue.verdict({"status": "error", "base_score": None}), "error")

    def test_add_and_record(self):
        with tempfile.TemporaryDirectory() as d:
            queue.main(["add", "rac4/ntsc", "func_00100000", "--dir", d, "--env", "A=1"])
            self.assertEqual(queue.items(Path(d))[0]["env"], "A=1")
            result = Path(d) / "result.json"
            result.write_text(json.dumps({"status": "matched", "base_score": 2, "best_score": 0,
                                          "best": {"artefacts": [], "function": "void f(void) {}\n"}}))
            item = queue.items(Path(d))[0]
            self.assertEqual(queue.record(Path(d), item, result, 12.0), "WIN")
            self.assertEqual(queue.done_keys(Path(d)), {("rac4/ntsc", "func_00100000")})
            self.assertTrue((Path(d) / "wins" / "rac4-ntsc" / "func_00100000.c").is_file())


class UpstreamTest(unittest.TestCase):
    def test_patches_are_stamped(self):
        stamp = upstream.patch_stamp()
        self.assertIn("asm-passes.patch", stamp)
        self.assertIn("aligned-scorer.patch", stamp)


@unittest.skipUnless((upstream.cache_dir() / f"decomp-permuter-{upstream.COMMIT[:7]}").is_dir(),
                     "no decomp-permuter checkout (python3 tools/permuter fetch)")
class AcceptTest(unittest.TestCase):
    def setUp(self):
        upstream.load(upstream.cache_dir() / f"decomp-permuter-{upstream.COMMIT[:7]}")

    def test_artefacts_new_in_the_candidate(self):
        base = "int g(int);\nint f(int a) {\n    int x;\n    x = g(a);\n    return x;\n}\n"
        dup = "int g(int);\nint f(int a) {\n    int x;\n    if (a) { x = g(a); } else { x = g(a); }\n    return x;\n}\n"
        dead = "int g(int);\nint f(int a) {\n    int x;\n    x = 1;\n    x = g(a);\n    return x;\n}\n"
        uninit = "int g(int);\nint f(int a) {\n    int x;\n    int y;\n    x = g(y);\n    return x;\n}\n"
        self.assertEqual(accept.artefacts(base, base, "f"), [])
        self.assertTrue(accept.artefacts(base, dup, "f")[0].startswith("duplicated call: g(a)"))
        self.assertTrue(accept.artefacts(base, dead, "f")[0].startswith("dead store: `x = 1`"))
        self.assertEqual(accept.artefacts(base, uninit, "f"), ["uninitialised read: y is read before it is set"])


if __name__ == "__main__":
    unittest.main()
