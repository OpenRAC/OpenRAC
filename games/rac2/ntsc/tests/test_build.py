"""Object-cache and release-check regressions, without game data or toolchains."""
from contextlib import ExitStack, redirect_stdout
from io import StringIO
import json
import os
import shlex
import struct
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import build


class RelocationValidation(unittest.TestCase):
    def derive(self, mine, retail, relocs, symbol):
        data = struct.pack("<" + "I" * len(mine), *mine)
        ref = struct.pack("<" + "I" * len(retail), *retail)
        image = {"data": ref, "segments": [{"type": 1, "address": 0x200000,
                  "offset": 0, "filesz": len(ref)}]}
        obj = {"sections": [{"data": data}], "symbols": [{"name": symbol, "type": 1}],
               "relocations": {0: [(offset, kind, 0) for offset, kind in relocs]}}
        with patch.object(build, "program_functions", return_value={}):
            return build.Placement("boot", image).derive(obj, 0, 0x200000)

    def test_hi_lo_relocations_still_validate_registers_and_opcode(self):
        relocations = [(0, build.elf.R_MIPS_HI16), (4, build.elf.R_MIPS_LO16)]
        values, error = self.derive([0x3C020000, 0x8C440000], [0x3C020014, 0x8C449568],
                                    relocations, "D_00139568")
        self.assertIsNone(error)
        self.assertEqual(values["D_00139568"], 0x139568)
        for retail, part in (([0x3C030014, 0x8C449568], "%hi"),
                             ([0x3C020014, 0x8C649568], "%lo")):
            self.assertIn(part, self.derive([0x3C020000, 0x8C440000], retail,
                                           relocations, "D_00139568")[1])

    def test_hi_pairs_with_the_lo_after_it_in_the_object_even_at_a_lower_address(self):
        # A %hi in a loop's last delay slot, paired by the assembler with the %lo at the top.
        relocations = [(4, build.elf.R_MIPS_HI16), (0, build.elf.R_MIPS_LO16)]
        values, error = self.derive([0x8C440000, 0x3C020000], [0x8C449568, 0x3C020014],
                                    relocations, "D_00139568")
        self.assertIsNone(error)
        self.assertEqual(values["D_00139568"], 0x139568)
        self.assertIn("%hi", self.derive([0x8C440000, 0x3C020000], [0x8C449568, 0x3C030014],
                                         relocations, "D_00139568")[1])

    def test_gp_and_jump_relocations_do_not_hide_changed_instructions(self):
        self.assertIn("gp access", self.derive([0x8F820000], [0x8F830000],
            [(0, build.elf.R_MIPS_GPREL16)], "D_001AEFF0")[1])
        self.assertIn("jump", self.derive([0x0C000000], [0x08048000],
            [(0, build.elf.R_MIPS_26)], "func_00120000")[1])


class ObjectCache(unittest.TestCase):
    def setUp(self):
        self.stack = ExitStack()
        self.addCleanup(self.stack.close)
        self.root = Path(self.stack.enter_context(tempfile.TemporaryDirectory()))
        self.stack.enter_context(patch.multiple(build, ROOT=self.root, BUILD=self.root / "build",
            COMPILER=self.root / "build/compiler/tools", SDK_COMPILER=self.root / "build/compiler/sdk"))
        self.stack.enter_context(patch.dict(os.environ, {"RAC2_LINUX": ""}))
        self.source = self.root / "src/game/example.c"
        self.write(self.source, "int example(void) { return 1; }\n")
        self.write(self.root / "include/example.h", "#define EXAMPLE 1\n")
        self.write(self.source.parent / "local.h", "#define LOCAL 1\n")
        for directory in (build.COMPILER, build.SDK_COMPILER):
            for name in ("cpp", "cc1", "as", "xgcc"):
                self.write(directory / name, name)
            self.write(directory / "include/stdarg.h", "/* compiler header */\n")
        self.out = build.BUILD / "obj/src/game/example.c.o"
        self.run_process = build.subprocess.run
        self.compiler = self.stack.enter_context(patch.object(build.subprocess, "run", side_effect=self.compile))

    def write(self, path, contents):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents)

    def compile(self, *args, **kwargs):
        self.out.write_bytes(b"compiled object")
        return SimpleNamespace(returncode=0, stderr="")

    def test_unchanged_inputs_reuse_object(self):
        build.compile_c(self.source)
        build.compile_c(self.source)
        self.assertEqual(self.compiler.call_count, 1)

    def test_sdata_metadata_runs_before_assembler_in_both_chains(self):
        for prefix in ("", "sdk "):
            self.write(self.root / "config/cflags.txt", "src/game/example.c\t" + prefix + "-O2\n")
            build.compile_c(self.source)
            commands = self.commands()
            metadata = next(i for i, c in enumerate(commands)
                            if c[:2] == ["python3", str(build.SDATA_ALIASES)])
            assembler = next(i for i, c in enumerate(commands) if c[0] == str(build.COMPILER / "as"))
            self.assertEqual(commands[metadata][2:], ["x.s"])
            self.assertLess(metadata, assembler)

    def test_sdata_transformer_content_invalidates_the_object(self):
        script = self.root / "tools/sdata_aliases.py"
        self.write(script, build.SDATA_ALIASES.read_text())
        with patch.object(build, "SDATA_ALIASES", script):
            build.compile_c(self.source)
            before = self.compiler.call_count
            script.write_text(script.read_text() + "\n# changed metadata support\n")
            build.compile_c(self.source)
            self.assertEqual(self.compiler.call_count, before + 1)
            build.compile_c(self.source)
            self.assertEqual(self.compiler.call_count, before + 1)

    def commands(self):
        lexer = shlex.shlex(self.compiler.call_args.args[0][-1], posix=True, punctuation_chars=";")
        lexer.whitespace_split = True
        commands = [[]]
        for token in lexer:
            if token == ";":
                commands.append([])
            else:
                commands[-1].append(token)
        return [command for command in commands if command]

    def test_assembler_options_reach_only_assembler_in_both_chains(self):
        for prefix, compiler in (("", build.COMPILER / "cc1"), ("sdk ", build.SDK_COMPILER / "xgcc")):
            with self.subTest(prefix=prefix):
                self.write(self.root / "config/cflags.txt", "src/game/example.c\t" + prefix
                    + "-O2 -G2 -Wa,--r5900-sign-bit=unsigned -Wa,--defsym,example=7\n")
                build.compile_c(self.source)
                commands = self.commands()
                frontend = next(command for command in commands
                                if command[0].replace("\\", "/") == compiler.as_posix())
                assembler = next(command for command in commands if command[0] == str(build.COMPILER / "as"))
                self.assertIn("-O2", frontend)
                self.assertIn("-G2", frontend)
                self.assertFalse(any(arg.startswith("-Wa") or arg.startswith("--r5900")
                                     or arg in ("--defsym", "example=7") for arg in frontend))
                # SDK units also pool li.d constants (--r5900-pool-li-d); game units do not.
                pool = ["--r5900-pool-li-d"] if prefix else []
                self.assertEqual(assembler[1:], pool + ["--r5900-sign-bit=unsigned", "--defsym",
                                                         "example=7", "-o", "x.o", "x.s"])

    def test_default_assembler_arguments_and_option_cache_changes(self):
        build.compile_c(self.source)
        assembler = next(command for command in self.commands() if command[0] == str(build.COMPILER / "as"))
        self.assertEqual(assembler[1:], ["-o", "x.o", "x.s"])
        for mode in ("unsigned", "signed"):
            before = self.compiler.call_count
            self.write(self.root / "config/cflags.txt", "src/game/example.c\t-O2 -Wa,--r5900-sign-bit=" + mode + "\n")
            build.compile_c(self.source)
            self.assertEqual(self.compiler.call_count, before + 1)
            build.compile_c(self.source)
            self.assertEqual(self.compiler.call_count, before + 1)

    def test_empty_assembler_arguments_fail_before_compiling(self):
        for arg in ("-Wa", "-Wa,", "-Wa,,--example", "-Wa,--example,", "-Wa,--example,,value"):
            for prefix in ("", "sdk "):
                with self.subTest(arg=arg, prefix=prefix):
                    self.write(self.root / "config/cflags.txt", "src/game/example.c\t" + prefix + arg + "\n")
                    with self.assertRaisesRegex(SystemExit, "nonempty comma-separated assembler arguments"):
                        build.compile_c(self.source)
        self.compiler.assert_not_called()
        self.assertFalse(self.out.with_suffix(".inputs").exists())

    @unittest.skipIf(os.name == "nt", "The build shell runs in Linux; run this test there too")
    def test_real_shell_preserves_quoted_arguments_for_both_chains(self):
        # Authored stand-ins record the actual argv received through the build shell.
        for directory, name, output in ((build.COMPILER, "cpp", ""),
                                        (build.COMPILER, "cc1", "x.s"),
                                        (build.SDK_COMPILER, "xgcc", "x.s"),
                                        (build.COMPILER, "as", "x.o")):
            tool = directory / name
            script = "#!/bin/bash\nprintf '%s\\n' \"$@\" > " + shlex.quote(str(tool.with_suffix(".argv"))) + "\n"
            if output:
                script += "printf 'authored output' > " + output + "\n"
            self.write(tool, script)
            tool.chmod(0o755)
        self.compiler.side_effect = self.run_process
        literal = "--note=space ' quote; $(printf BAD)"
        for prefix, compiler in (("", build.COMPILER / "cc1"), ("sdk ", build.SDK_COMPILER / "xgcc")):
            with self.subTest(prefix=prefix):
                self.write(self.root / "config/cflags.txt", "src/game/example.c\t" + prefix + "-O2 "
                    + shlex.quote("-DNAME=two words") + " " + shlex.quote("-Wa," + literal) + "\n")
                build.compile_c(self.source)
                pool = ["--r5900-pool-li-d"] if prefix else []
                self.assertEqual((build.COMPILER / "as.argv").read_text().splitlines(),
                                 pool + [literal, "-o", "x.o", "x.s"])
                frontend = compiler.with_suffix(".argv").read_text().splitlines()
                self.assertIn("-DNAME=two words", frontend)
                self.assertNotIn(literal, frontend)
                self.assertEqual(self.out.read_bytes(), b"authored output")

    def test_changed_executables_invalidate_even_with_preserved_mtime_and_size(self):
        for name in ("cpp", "cc1", "as"):
            with self.subTest(name=name):
                build.compile_c(self.source)
                before = self.compiler.call_count
                path = build.COMPILER / name
                stat = path.stat()
                replacement = path.with_suffix(".new")
                replacement.write_bytes(b"X" * stat.st_size)
                os.utime(replacement, ns=(stat.st_atime_ns, stat.st_mtime_ns))
                os.replace(replacement, path)
                build.compile_c(self.source)
                self.assertEqual(self.compiler.call_count, before + 1)

    def test_sdk_tracks_driver_frontend_preprocessor_and_game_assembler(self):
        self.write(self.root / "config/cflags.txt", "src/game/example.c\tsdk -O2\n")
        for path in [build.SDK_COMPILER / name for name in ("xgcc", "cpp", "cc1")] + [build.COMPILER / "as"]:
            with self.subTest(path=path):
                build.compile_c(self.source)
                before = self.compiler.call_count
                path.write_bytes(path.read_bytes() + b"new")
                build.compile_c(self.source)
                self.assertEqual(self.compiler.call_count, before + 1)
        before = self.compiler.call_count
        (build.COMPILER / "cc1").write_text("unselected game compiler changed")
        build.compile_c(self.source)
        self.assertEqual(self.compiler.call_count, before)

    def test_source_headers_flags_and_runner_invalidate(self):
        for path in (self.source, self.root / "include/example.h", self.source.parent / "local.h",
                     build.COMPILER / "include/stdarg.h"):
            with self.subTest(path=path):
                build.compile_c(self.source)
                before = self.compiler.call_count
                path.write_text(path.read_text() + "\n")
                build.compile_c(self.source)
                self.assertEqual(self.compiler.call_count, before + 1)
        before = self.compiler.call_count
        self.write(self.root / "config/cflags.txt", "src/game/example.c\t-O1\n")
        build.compile_c(self.source)
        self.assertEqual(self.compiler.call_count, before + 1)
        with patch.dict(os.environ, {"RAC2_LINUX": "container exec compiler"}):
            build.compile_c(self.source)
        self.assertEqual(self.compiler.call_count, before + 2)

    def test_old_flags_only_cache_is_rebuilt(self):
        self.write(self.out, "old object")
        self.write(self.out.with_suffix(".flags"), build.flags_for(self.source))
        build.compile_c(self.source)
        self.assertEqual(self.compiler.call_count, 1)

    def test_missing_tool_does_not_reuse_cached_object(self):
        build.compile_c(self.source)
        (build.COMPILER / "as").unlink()
        with self.assertRaises(FileNotFoundError):
            build.compile_c(self.source)

    def test_failed_compile_does_not_stamp_new_inputs(self):
        build.compile_c(self.source)
        stamp = self.out.with_suffix(".inputs")
        previous = stamp.read_text()
        self.source.write_text("not C")
        self.compiler.side_effect = None
        self.compiler.return_value = SimpleNamespace(returncode=1, stderr="syntax error")
        with self.assertRaisesRegex(SystemExit, "syntax error"):
            build.compile_c(self.source)
        self.assertEqual(stamp.read_text(), previous)


class StrictBuild(unittest.TestCase):
    def exercise(self, strict, differs, same=True):
        with tempfile.TemporaryDirectory() as directory, ExitStack() as stack:
            root = Path(directory)
            placement = SimpleNamespace(functions={}, values={}, placed={}, differ=differs, place=lambda: None)

            def configure(reference, out, name):
                (out / "config").mkdir(parents=True)
                (out / "config/rac2.ld").write_text("SECTIONS {}")
                return {}

            stack.enter_context(patch.object(build, "BUILD", root))
            stack.enter_context(patch.object(build, "splat_config", side_effect=configure))
            stack.enter_context(patch.object(build, "Placement", return_value=placement))
            stack.enter_context(patch.object(build.elf, "read", return_value={"entry": 0}))
            stack.enter_context(patch.object(build.elf, "compare_loads", return_value=(same, "synthetic comparison")))
            for name in ("run", "adapt_linker", "undefined_symbols"):
                stack.enter_context(patch.object(build, name))
            stack.enter_context(redirect_stdout(StringIO()))
            error = None
            try:
                build.build_program("boot", root / "reference", strict=strict)
            except SystemExit as exc:
                error = exc.code
            report = root / "boot/placed.json"
            return error, json.loads(report.read_text()) if report.exists() else None

    def test_strict_rejects_fallback_even_when_loaded_bytes_match(self):
        error, report = self.exercise(True, [("candidate", "wrong size")])
        self.assertIn("strict check failed", error)
        self.assertEqual(report["differ"], {"candidate": "wrong size"})

    def test_exploratory_build_retains_fallback(self):
        error, report = self.exercise(False, [("candidate", "wrong size")])
        self.assertIsNone(error)
        self.assertEqual(report["differ"], {"candidate": "wrong size"})

    def test_strict_accepts_no_fallback_and_rejects_loaded_byte_changes(self):
        self.assertEqual(self.exercise(True, [])[0], None)
        error, report = self.exercise(True, [], same=False)
        self.assertEqual(error, 1)
        self.assertIsNone(report)

    def test_parallel_build_propagates_strict_and_child_failure(self):
        with patch.object(sys, "argv", ["build.py", "all", "--strict", "--jobs", "2"]), \
             patch.dict(os.environ, {"RAC2_ONE_PREFIX": "1"}), \
             patch.object(build.levels, "NAMES", {0: "test"}), \
             patch.object(build.subprocess, "run", return_value=SimpleNamespace(
                 returncode=1, stdout="", stderr="strict failure")) as run, redirect_stdout(StringIO()):
            with self.assertRaisesRegex(SystemExit, "2 of 2 programs failed"):
                build.main()
            self.assertEqual(run.call_count, 2)
            for call in run.call_args_list:
                self.assertIn("--strict", call.args[0])

    def test_parallel_build_gives_each_program_its_own_wine_prefix(self):
        # One wineserver serves a prefix, so programs at once each get one of their own.
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(sys, "argv", ["build.py", "all", "--jobs", "2"]), \
             patch.dict(os.environ, {}, clear=False), \
             patch.object(build, "BUILD", Path(tmp)), \
             patch.object(build.levels, "NAMES", {0: "test"}), \
             patch.object(build.subprocess, "run", return_value=SimpleNamespace(
                 returncode=0, stdout="", stderr="")) as run, redirect_stdout(StringIO()):
            os.environ.pop("WINEPREFIX", None)
            os.environ.pop("RAC2_ONE_PREFIX", None)
            build.main()
            boots = [c for c in run.call_args_list if "wineboot" in c.args[0]]
            programs = [c for c in run.call_args_list if "wineboot" not in c.args[0]]
            self.assertEqual(len(boots), 2)
            self.assertEqual(len(programs), 2)
            used = {c.kwargs["env"]["WINEPREFIX"] for c in programs}
            self.assertTrue(used <= {str(Path(tmp) / "wine" / n) for n in ("0", "1")})
            for c in programs:
                self.assertTrue(c.kwargs["env"]["WINEPREFIX"].startswith(str(Path(tmp) / "wine")))

    def test_a_set_wine_prefix_keeps_one_prefix(self):
        with patch.dict(os.environ, {"WINEPREFIX": "/somewhere"}):
            self.assertIsNone(build.wine_prefixes(8))
        with patch.dict(os.environ, {"RAC2_ONE_PREFIX": "1"}):
            self.assertIsNone(build.wine_prefixes(8))
        self.assertIsNone(build.wine_prefixes(1))
