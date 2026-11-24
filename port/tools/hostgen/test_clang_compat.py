# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile translated anonymous typedefs using the installed host Clang.

This needs no POSIX runtime harness and also exercises Windows source paths.
"""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest import mock

import clangast
import hostgen


class ClangAliases(unittest.TestCase):
    def test_size_and_ptrdiff_aliases_are_the_targets_types(self):
        # Fedora's Clang writes memmove's parameter as __size_t, with no declaration and, for an
        # implicit builtin's, no desugared type; also inside the function's own type.
        ast = {"kind": "TranslationUnitDecl", "inner": [
            {"kind": "FunctionDecl", "name": "memmove",
             "type": {"qualType": "void *(void *, const void *, __size_t)"},
             "inner": [{"kind": "ParmVarDecl", "type": {"qualType": "__size_t"}},
                       {"kind": "ParmVarDecl", "type": {"qualType": "__ptrdiff_t *"}},
                       {"kind": "ParmVarDecl", "type": {"qualType": "my__size_t"}}]},
        ]}
        clangast.annotate(ast)
        fn = ast["inner"][0]
        self.assertEqual(fn["type"]["qualType"], "void *(void *, const void *, unsigned int)")
        self.assertEqual([p["type"]["qualType"] for p in fn["inner"]], ["unsigned int", "int *", "my__size_t"])


class ClangRunsWithoutACompilerCache(unittest.TestCase):
    def test_ccache_cannot_answer_for_a_rewritten_prepared_copy(self):
        # clang on PATH may be ccache's wrapper. Its direct cache keys a file by inode and
        # second-granular mtime, and hostgen rewrites its prepared copies between checks, so a
        # hit can report the previous content's verdict (it hid pause.c's conflicting types
        # from the syntax check, where the AST dump then refused the same file).
        with mock.patch.object(clangast.subprocess, "run") as run:
            run.return_value = subprocess.CompletedProcess([], 0, "", "")
            clangast.run("clang", Path("src/game/pause.c"), Path("/tmp"), ["-fsyntax-only"], False)
        env = run.call_args.kwargs["env"]
        self.assertEqual(env.get("CCACHE_DISABLE"), "1")


@unittest.skipUnless(shutil.which("clang"), "needs Clang")
class AsmStubsKeepLines(unittest.TestCase):
    def test_a_break_right_after_asm_is_kept(self):
        # `__asm__ (` alone on its line is shorter than the stub call: the break after it was
        # written over, every later line moved up one, and the fixes of later diagnostics went to
        # the line above (rac3's L07_003C7468.c, whose definitions after the assembly then took
        # their labels on their first local).
        lines = ["void f(void) {", "    __asm__ (", '        "vsub.xyz $vf2, $vf2, $vf1\\n"',
                 '        : : : "vf1");', "    g();", "}"]
        self.assertTrue(clangast._replace_asm(lines, 3, 12))  # at a constraint, as Clang points
        self.assertEqual(len(lines), 6)
        self.assertEqual(lines[1].strip(), "openrac_hostgen_asm();")
        self.assertEqual(lines[4], "    g();")

    def test_a_one_line_statement_stays_on_its_line(self):
        lines = ['    __asm__ volatile ("sync.l" : : : "memory");', "    g();"]
        self.assertTrue(clangast._replace_asm(lines, 0, 30))
        self.assertEqual(lines[0].strip(), "openrac_hostgen_asm();")
        self.assertEqual(lines[1], "    g();")


class Vu0InlineAssemblyAsC(unittest.TestCase):
    # rac3's func_L01_00446BF0 (L01_004463D0.c): two file-local helpers of VU0 macro-mode
    # assembly on GCC's "j" registers. As stubs they never moved the stepped line test's points.
    ADD_COPY = ('__asm__("lqc2 %0, %1" : "=j"(x) : "m"(*(vu0_v4 *)b)); '
                '__asm__("lqc2 %0, %1" : "=j"(y) : "m"(*(vu0_v4 *)a)); '
                '__asm__("vadd.xyz %0, %0, %1" : "+j"(y) : "j"(x)); '
                '__asm__("lqc2 %0, %1" : "+j"(x) : "m"(*(vu0_v4 *)cs)); '
                '__asm__("sqc2 %1, %0" : "=m"(*(vu0_v4 *)cd) : "j"(x)); '
                '__asm__("sqc2 %1, %0" : "=m"(*(vu0_v4 *)d) : "j"(y));')
    ADD_L = ('__asm__("lqc2 %0, %1" : "=j"(qa) : "m"(*(vu0_v4 *)a)); '
             '__asm__("lqc2 %0, %1" : "=j"(qb) : "m"(*(vu0_v4 *)b), "j"(qa), "j"(qa)); '
             '__asm__("vadd.xyz %0, %1, %2" : "=j"(qr) : "j"(qa), "j"(qb)); '
             '__asm__("sqc2 %1, %0" : "=m"(*(vu0_v4 *)d) : "j"(qr));')

    def test_the_instructions_it_does_not_know_stay_stubs(self):
        self.assertIsNone(clangast.vu0_macro_c('__asm__("vitof0.xyzw %0, %1" : "=j"(f) : "j"(f));'))
        self.assertIsNone(clangast.vu0_macro_c('__asm__("lqc2 %0, D_L01_001D6750" : "=j"(qa));'))
        self.assertIsNone(clangast.vu0_macro_c('__asm__("vsub.xyz $vf2, $vf2, $vf1" : : : "vf1");'))
        self.assertIsNone(clangast.vu0_macro_c('__asm__ volatile ("sync.l" : : : "memory");'))

    def test_replacing_writes_c_on_the_statements_lines(self):
        lines = ["void f(void *d, void *a) {", "    vu0_q x;",
                 '    __asm__("lqc2 %0, %1"', '            : "=j"(x) : "m"(*(vu0_v4 *)a));',
                 '    __asm__("sqc2 %1, %0" : "=m"(*(vu0_v4 *)d) : "j"(x));', "}"]
        self.assertTrue(clangast._replace_asm(lines, 3, 15))
        self.assertEqual(len(lines), 6)
        self.assertIn(clangast.VU0_C_MARK, lines[2])
        self.assertNotIn("openrac_hostgen_asm", "\n".join(lines))
        # The next statement is its own: a place inside the written C is not taken for it.
        self.assertFalse(clangast._replace_asm(lines, 2, len(lines[2]) - 2))

    def test_c_clang_refuses_goes_back_to_the_stub(self):
        line = "    " + clangast.vu0_macro_c('__asm__("lqc2 %0, %1" : "=j"(x) : "m"(*(vu0_v4 *)a));') + " g();"
        lines = [line]
        self.assertTrue(clangast._unwrite_vu0_c(lines, 0, 30))
        self.assertEqual(lines[0], "    openrac_hostgen_asm(); g();")

    @unittest.skipUnless(shutil.which("clang"), "needs Clang")
    def test_the_c_computes_what_the_instructions_do(self):
        c = lambda stmts: " ".join(clangast.vu0_macro_c(s.strip() + ";") for s in stmts.split(";") if s.strip())
        program = (
            "#include <stdio.h>\n"
            "typedef int vu0_q __attribute__((mode(TI)));\n"
            "typedef struct { float x, y, z, w; } __attribute__((aligned(16))) vu0_v4;\n"
            "static void add_copy(void *d, void *a, void *b, void *cd, void *cs) { vu0_q x, y; " + c(self.ADD_COPY) + " }\n"
            "static void add_l(void *d, void *a, void *b) { vu0_q qa, qb, qr; " + c(self.ADD_L) + " }\n"
            "static void rot(void *d) { vu0_q q; " + c('__asm__("vmr32.xyzw %0, $vf0" : "=j"(q)); '
                                                    '__asm__("sqc2 %1, %0" : "=m"(*(vu0_v4 *)d) : "j"(q))') + " }\n"
            "static void neg(void *d, void *a) { vu0_q q, r; " + c('__asm__("lqc2 %0, %1" : "=j"(q) : "m"(*(vu0_v4 *)a)); '
                                                              '__asm__("vsub.xyz %0, $vf0, %1" : "=j"(r) : "j"(q)); '
                                                              '__asm__("sqc2 %1, %0" : "=m"(*(vu0_v4 *)d) : "j"(r))') + " }\n"
            "int main(void) {\n"
            "  vu0_v4 a = {1, 2, 3, 4}, b = {10, 20, 30, 40}, d, cd, cs = {7, 8, 9, 5}, e, r, n;\n"
            "  add_copy(&d, &a, &b, &cd, &cs); add_l(&e, &a, &b); rot(&r); neg(&n, &b);\n"
            "  printf(\"%g %g %g %g|%g %g %g %g|%g %g %g %g|%g %g %g %g|%g %g %g %g\\n\",\n"
            "         d.x, d.y, d.z, d.w, cd.x, cd.y, cd.z, cd.w, e.x, e.y, e.z, e.w, r.x, r.y, r.z, r.w,\n"
            "         n.x, n.y, n.z, n.w);\n"
            "  return 0;\n"
            "}\n")
        with tempfile.TemporaryDirectory(prefix="hostgen vu0 ") as tmp:
            src, exe = Path(tmp) / "vu0.c", Path(tmp) / "vu0"
            src.write_text(program)
            subprocess.run(["clang", "-std=gnu99", "-o", str(exe), str(src)], check=True)
            out = subprocess.run([str(exe)], check=True, capture_output=True, text=True).stdout.strip()
        self.assertEqual(out, "11 22 33 4|7 8 9 5|11 22 33 4|0 0 1 0|-10 -20 -30 40")


class ClangCompatibility(unittest.TestCase):
    def test_nested_source_and_anonymous_typedefs(self):
        with tempfile.TemporaryDirectory(prefix="hostgen compatibility ") as tmp:
            root = Path(tmp)
            source = root / "source"
            (source / "src/nested").mkdir(parents=True)
            (source / "src/nested/example.c").write_text(
                "typedef struct { int value; } Item;\n"
                "typedef union { Item item; int other; } Payload;\n"
                "typedef enum { VALUE = 7 } Kind;\n"
                "int func_00100000(Item *item, Item *end, Kind kind) {\n"
                "  Payload p; p.item.value = item->value;\n"
                "  return p.item.value + kind + sizeof(Payload) * item->value + (end - item);\n"
                "}\n"
            )
            config = root / "game.json"
            config.write_text(json.dumps({"name": "compatibility", "sources": ["src"],
                                          "includes": [], "defines": []}))
            output = root / "generated"
            self.assertEqual(hostgen.main([
                "--game", str(config), "--source", str(source), "--out", str(output),
                "--jobs", "2", "--quiet",
            ]), 0)
            generated = output / "nested/example.c"
            self.assertIn("func_00100000(", generated.read_text())
            report = json.loads((output / "report.json").read_text())
            self.assertEqual(report["units_unreadable"], [])
            self.assertEqual(report["index_problems"], [])
            runtime = Path(__file__).resolve().parents[2] / "runtime/include"
            compiled = subprocess.run([
                shutil.which("clang"), "-std=gnu11", "-fsyntax-only", "-I", str(runtime),
                "-I", str(output), str(generated),
            ], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)

    def test_gcc_label_and_redeclaration_leniencies(self):
        # What GCC 2.95 took and Clang refuses (rac1/ntsc's synced tree, 2026-10-15): a variable
        # given a second asm label (GCC kept the first), a label repeating a variable's name after
        # its use, a function labelled one way here and another way in a header included after
        # its use, and an extern of an anonymous struct contradicting a function-local one.
        with tempfile.TemporaryDirectory(prefix="hostgen leniencies ") as tmp:
            root = Path(tmp)
            source = root / "source"
            (source / "src").mkdir(parents=True)
            (source / "include").mkdir()
            (source / "include/fast_math.h").write_text('float absolute_float(float x) __asm__("FUN_001f99c0");\n')
            (source / "src/example.c").write_text(
                'extern float ref __asm__("D_0015ED6C");\n'
                'extern float table[];\n'
                'float absolute_float(float input) __asm__("func_001F99C0");\n'
                "int func_00100000(void) {\n"
                "  extern struct { char pad[0x18]; int moby; } D_00174140;\n"
                "  return (int)(absolute_float(table[0]) + D_00174140.moby);\n"
                "}\n"
                'extern float ref __asm__("D_0015EE6C");\n'
                'extern float table[] __asm__("table");\n'
                "extern struct { char pad[0x28]; float height; } D_00174140;\n"
                '#include "fast_math.h"\n'
                "float func_00100040(void) {\n"
                "  return ref * 2.0f + D_00174140.height + absolute_float(-1.0f);\n"
                "}\n"
            )
            config = root / "game.json"
            config.write_text(json.dumps({"name": "leniencies", "sources": ["src"],
                                          "includes": ["include"], "defines": []}))
            output = root / "generated"
            self.assertEqual(hostgen.main([
                "--game", str(config), "--source", str(source), "--out", str(output),
                "--jobs", "2", "--quiet",
            ]), 0)
            report = json.loads((output / "report.json").read_text())
            self.assertEqual(report["units_unreadable"], [])
            text = (output / "example.c").read_text()
            self.assertIn("func_00100040(", text)
            self.assertNotIn("D_0015EE6C", text)   # the first label is the variable's

    def test_a_local_that_repeats_a_parameters_name(self):
        # rac4-decomp's func_L01_003CA498 (2026-11): `u8 *object = *(u8 **)((u8 *)object + 0xAC);`
        # in a function whose parameter is `object`. GCC 2.95 only warned; the initializer reads
        # the parameter. The local is kept apart under an alias, with no asm label.
        with tempfile.TemporaryDirectory(prefix="hostgen shadow ") as tmp:
            root = Path(tmp)
            source = root / "source"
            (source / "src").mkdir(parents=True)
            (source / "include").mkdir()
            (source / "src/example.c").write_text(
                "int func_00100000(void *object) {\n"
                "    unsigned char *object = *(unsigned char **)((unsigned char *)object + 0xAC);\n"
                "    return object[4];\n"
                "}\n"
            )
            config = root / "game.json"
            config.write_text(json.dumps({"name": "shadow", "sources": ["src"],
                                          "includes": ["include"], "defines": []}))
            output = root / "generated"
            self.assertEqual(hostgen.main([
                "--game", str(config), "--source", str(source), "--out", str(output),
                "--jobs", "2", "--quiet",
            ]), 0)
            report = json.loads((output / "report.json").read_text())
            self.assertEqual(report["units_unreadable"], [])
            self.assertIn("func_00100000(", (output / "example.c").read_text())

    def test_an_array_of_a_record_never_completed(self):
        # rac2-decomp's L05_003008C0.c (d29456d6): the unit passes the array by address only.
        with tempfile.TemporaryDirectory(prefix="hostgen incomplete ") as tmp:
            root = Path(tmp)
            source = root / "source"
            (source / "src").mkdir(parents=True)
            (source / "include").mkdir()
            (source / "src/example.c").write_text(
                "typedef struct Glyph Glyph;\n"
                "extern int func_00100100(Glyph *);\n"
                "extern Glyph D_00200000[];\n"
                "int func_00100000(void) {\n"
                "    return func_00100100(D_00200000);\n"
                "}\n"
            )
            config = root / "game.json"
            config.write_text(json.dumps({"name": "incomplete", "sources": ["src"],
                                          "includes": ["include"], "defines": []}))
            output = root / "generated"
            self.assertEqual(hostgen.main([
                "--game", str(config), "--source", str(source), "--out", str(output),
                "--jobs", "2", "--quiet",
            ]), 0)
            report = json.loads((output / "report.json").read_text())
            self.assertEqual(report["units_unreadable"], [])
            self.assertIn("0x00200000", (output / "example.c").read_text())

    def test_a_definition_that_contradicts_a_shared_header(self):
        # A header shared by the callers says `void *` and `void` where the definitions take
        # `float *` and return int (rac1/pal's include/callees.h against its candidates): the
        # unit is read all the same, and each function is translated under its own symbol.
        with tempfile.TemporaryDirectory(prefix="hostgen contradiction ") as tmp:
            root = Path(tmp)
            source = root / "source"
            (source / "src").mkdir(parents=True)
            (source / "include").mkdir()
            (source / "include/callees.h").write_text(
                "extern void func_00100000(void *, void *);\n"
                "extern void func_00100040(void);\n"
            )
            (source / "src/example.c").write_text(
                '#include "callees.h"\n'
                "void func_00100000(float *out, float *in) {\n"
                "  out[0] = in[0] * 2.0f;\n"
                "}\n"
                "int func_00100040(void)\n"  # its brace on the next line
                "{\n"
                "  return 7;\n"
                "}\n"
                "int func_00100080(float *v) {\n"
                "  func_00100000(v, v);\n"
                "  return func_00100040();\n"
                "}\n"
            )
            config = root / "game.json"
            config.write_text(json.dumps({"name": "contradiction", "sources": ["src"],
                                          "includes": ["include"], "defines": []}))
            output = root / "generated"
            self.assertEqual(hostgen.main([
                "--game", str(config), "--source", str(source), "--out", str(output),
                "--jobs", "2", "--quiet",
            ]), 0)
            report = json.loads((output / "report.json").read_text())
            self.assertEqual(report["units_unreadable"], [])
            generated = output / "example.c"
            text = generated.read_text()
            for name in ("func_00100000(", "func_00100040(", "func_00100080("):
                self.assertIn(name, text)
            self.assertNotIn("__openrac_decl_", text)  # the aliases stay in the prepared copy
            runtime = Path(__file__).resolve().parents[2] / "runtime/include"
            compiled = subprocess.run([
                shutil.which("clang"), "-std=gnu11", "-fsyntax-only", "-I", str(runtime),
                "-I", str(output), str(generated),
            ], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)


    def test_a_third_contradicting_declaration_keeps_the_functions_symbol(self):
        # Veldin's L01_0035F480.c declares the vector scale func_0038CE20 three ways: the second
        # became an alias (func_0038CE20__openrac_decl_140) labelled func_0038CE20, the third was
        # renamed with it, and on Clang's next pass it contradicted the alias and was labelled with
        # the alias's name. Its calls were taken for the executable's func_0038CE20 (another
        # routine at that address) rather than the level's copy. Every alias is labelled with the
        # function's own symbol.
        text = (
            "void f(void *v) { extern void func_0038CE20__openrac_decl_2(void *, void *, float) __asm__(\"func_0038CE20\"); }\n"
            "extern void func_0038CE20__openrac_decl_2(long *, long *, float);\n"
            "void g(long *v) { func_0038CE20__openrac_decl_2(v, v, 4.0f); }\n")
        stderr = ("src/b.c:2:13: error: conflicting types for 'func_0038CE20__openrac_decl_2'\n"
                  "src/b.c:1:32: note: previous declaration is here\n")
        fixed, fixes = clangast.fix_from_diagnostics(text, stderr, "src/b.c")
        self.assertIn('func_0038CE20__openrac_decl_2__openrac_decl_2(long *, long *, float) __asm__("func_0038CE20");',
                      fixed)
        self.assertIn("func_0038CE20__openrac_decl_2__openrac_decl_2(v, v, 4.0f)", fixed)

    def test_an_override_replaces_matched_c_in_the_port_only(self):
        # The port's own C for a function the decompilation matches (hostgen.json "overrides"):
        # the matched definition is left out of the prepared copy, its lines kept, and the
        # override is translated in its place; it may differ in its signature (a result the
        # matching C declares void). A name it lists that nothing defines is an index problem.
        with tempfile.TemporaryDirectory(prefix="hostgen overrides ") as tmp:
            root = Path(tmp)
            source = root / "source"
            (source / "src").mkdir(parents=True)
            matched = (
                "typedef struct { float v[4]; } V;\n"
                "extern float func_00100080(V *, V *);\n"
                "void func_00100000(V *a, V *b, float r) {\n"
                "    if (r > 0.0f) {\n"
                "        func_00100080(a, b);\n"
                "    }\n"
                "}\n"
                "extern float func_00100000_f(V *, V *, float) __asm__(\"func_00100000\");\n"
                "float func_00100040(V *a, V *b) {\n"
                "    return func_00100000_f(a, b, 1.0f) * 2.0f;\n"
                "}\n"
            )
            (source / "src/example.c").write_text(matched)
            config = root / "game.json"
            config.write_text(json.dumps({"name": "overrides", "sources": ["src"], "includes": [], "defines": [],
                                          "overrides": "overrides.tsv"}))
            (root / "overrides.tsv").write_text("# name\tfile\nfunc_00100000\tmine.c\nfunc_00100100\tmine.c\n")
            (root / "mine.c").write_text(
                "/* the distance, which retail leaves in $f0 */\n"
                "extern float func_00100080(V *, V *);\n"
                "float func_00100000(V *a, V *b, float r) {\n"
                "    if (r > 0.0f) {\n"
                "        return func_00100080(a, b);\n"
                "    }\n"
                "    return 0.0f;\n"
                "}\n"
            )
            output = root / "generated"
            self.assertEqual(hostgen.main([
                "--game", str(config), "--source", str(source), "--out", str(output),
                "--jobs", "2", "--quiet",
            ]), 0)
            report = json.loads((output / "report.json").read_text())
            self.assertEqual(report["units_unreadable"], [])
            self.assertEqual(report["overrides_used"], ["func_00100000"])
            self.assertIn("overrides: func_00100100 has no matched definition to replace", report["index_problems"])
            prepared = (output / "prep/src/example.c").read_text()
            lines = prepared.split("\n")
            self.assertEqual(lines[2:7], ["", "", "", "", ""])          # the matched definition, its lines kept
            self.assertEqual(lines[8], "float func_00100040(V *a, V *b) {")
            self.assertEqual((source / "src/example.c").read_text(), matched)   # the decompilation untouched
            text = (output / "example.c").read_text()
            self.assertIn("float func_00100000(", text)
            # the caller reads a real result: no call site that reads a void function's
            self.assertNotIn("func_00100000:", json.dumps(report["call_site_mismatches"]))
            runtime = Path(__file__).resolve().parents[2] / "runtime/include"
            compiled = subprocess.run([
                shutil.which("clang"), "-std=gnu11", "-fsyntax-only", "-I", str(runtime),
                "-I", str(output), str(output / "example.c"),
            ], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)


if __name__ == "__main__":
    unittest.main()
