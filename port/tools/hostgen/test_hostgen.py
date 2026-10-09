# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""hostgen's tests: small programs in the console's C, translated, compiled
for the host with a minimal runtime (tests/harness.c), run, and their output
checked. Each program is what the decompilation does in some function; none
is game code.

    python3 -m unittest discover -s port/tools/hostgen
"""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import ctype  # noqa: E402
import hostgen  # noqa: E402
import prep  # noqa: E402

RUNTIME_INCLUDE = HERE.parent.parent / "runtime" / "include"

COMMON_H = """
#ifndef COMMON_H
#define COMMON_H
typedef signed char s8;
typedef short s16;
typedef int s32;
typedef long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef float f32;
extern void test_print(int);
extern void test_print_float(float);
extern void test_print_str(char *);
static __inline__ void qcopy(void *dst, void *src) {
    __asm__ __volatile__("lq $2,0x0(%1)\\n\\tsq $2,0x0(%0)" : : "r"(dst), "r"(src) : "$2", "memory");
}
#endif
"""

HOST_FUNCTIONS = ["test_print", "test_print_float", "test_print_str"]


def have_compiler() -> bool:
    return shutil.which("clang") is not None and sys.platform != "win32"


@unittest.skipUnless(have_compiler(), "needs Clang on a POSIX host")
class Translate(unittest.TestCase):
    def run_program(self, files: dict[str, str], places: str = "") -> list[str]:
        """Translates, builds and runs files (name -> C); returns the printed lines."""
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            src = tmp / "game"
            (src / "include").mkdir(parents=True)
            (src / "include" / "common.h").write_text(COMMON_H)
            for name, text in files.items():
                path = src / "src" / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('#include "common.h"\n' + textwrap.dedent(text))
            cfg = {"name": "test", "sources": ["src"], "includes": ["include", "src"],
                   "host_functions": HOST_FUNCTIONS}
            if places:
                (src / "functions.tsv").write_text(places)
                cfg["places"] = "functions.tsv"
            (tmp / "hostgen.json").write_text(json.dumps(cfg))
            out = tmp / "gen"
            rc = hostgen.main(["--game", str(tmp / "hostgen.json"), "--source", str(src), "--out", str(out),
                               "--jobs", "2", "--quiet"])
            self.assertEqual(rc, 0)
            report = json.loads((out / "report.json").read_text())
            self.assertEqual(report["functions_stubbed"], 0, report["stubbed"])
            sources = [out / u.strip() for u in (out / "units.txt").read_text().splitlines()]
            sources += [out / "stubs.c", out / "functions.c", HERE / "tests" / "harness.c"]
            exe = tmp / "prog"
            build = subprocess.run(
                ["clang", "-std=gnu11", "-O1", "-w", "-fno-strict-aliasing", "-fwrapv", "-fsigned-char",
                 "-I", str(RUNTIME_INCLUDE), "-I", str(out), "-o", str(exe), *map(str, sources)],
                capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=30)
            self.assertEqual(run.returncode, 0, run.stderr)
            return run.stdout.split()

    def test_globals_structs_and_pointer_arithmetic(self):
        out = self.run_program({"a.c": """
            typedef struct Node { struct Node *next; short id; char pad; float f; s64 big; int arr[3]; } Node;
            extern Node D_00200000[4];
            extern int D_00200100;
            int test_main(void) {
                Node *n = D_00200000;
                Node *last = &D_00200000[3];
                int i;
                for (i = 0; i < 4; i++) {
                    n[i].id = i * 10;
                    n[i].next = i < 3 ? &n[i + 1] : 0;
                    n[i].big = (s64)i << 40;
                }
                test_print(sizeof(Node));            /* the console's layout: big is 8-aligned, 40 in all */
                test_print(last - n);                /* pointer difference, in elements */
                test_print(n->next->next->id);
                test_print((int)(n[3].big >> 40));
                test_print(*(short *)((char *)D_00200000 + 40 * 2 + 4));  /* raw offset access */
                D_00200100 = (int)&D_00200000[1];
                test_print(D_00200100 - 0x200000);   /* addresses are the console's */
                n++;
                test_print(n->id);
                test_print(n[-1].id);
                return 0;
            }
        """})
        self.assertEqual(out, ["40", "3", "20", "3", "20", "40", "10", "0"])

    def test_locals_in_game_memory(self):
        out = self.run_program({"a.c": """
            void fill(int *p, int n) { int i; for (i = 0; i < n; i++) p[i] = i * i; }
            void set(float *f) { *f = 2.5f; }
            typedef struct { int a; int b[2]; } Pair;
            int sum(Pair *p) { return p->a + p->b[0] + p->b[1]; }
            int test_main(void) {
                int arr[5];
                float x = 1.0f;
                Pair pr;
                int total = 0;
                int i;
                fill(arr, 5);
                for (i = 0; i < 5; i++) total += arr[i];
                test_print(total);
                set(&x);
                test_print_float(x);
                pr.a = 1; pr.b[0] = 2; pr.b[1] = 3;
                test_print(sum(&pr));
                test_print(sum(&(Pair){4, {5, 6}}));
                return 0;
            }
        """})
        self.assertEqual(out, ["30", "2.500", "6", "15"])

    def test_calls_follow_the_definition(self):
        # The caller declares the callee with its float and integer arguments
        # swapped, and with one argument fewer: on the EE that call still
        # works (floats and integers travel in separate registers; the
        # missing one is the caller's own, still in its register).
        out = self.run_program({
            "callee.c": """
                int mix(int a, float b, int c) { test_print(a); test_print_float(b); return c; }
            """,
            "caller.c": """
                extern int mix(float, int);
                int test_main(void) { return run(7); }
                int run(int unused) { test_print(mix(1.5f, 3)); return 0; }
            """,
        })
        self.assertEqual(out, ["3", "1.500", "0"])

    def test_pass_through(self):
        out = self.run_program({
            "callee.c": "int two(int a, int b) { return a * 10 + b; }",
            "caller.c": """
                extern int two(int);
                int wrap(int x, int y) { return two(x); }
                int test_main(void) { test_print(wrap(4, 2)); return 0; }
            """,
        })
        self.assertEqual(out, ["42"])

    def test_function_pointers_by_code_address(self):
        out = self.run_program({"a.c": """
            int func_00110000(int x) { return x + 1; }
            int func_L05_00300000(int x) { return x + 500; }
            typedef int (*Fn)(int);
            extern Fn D_00200000[2];
            int test_main(void) {
                D_00200000[0] = func_00110000;
                D_00200000[1] = (Fn)0x00300000;
                test_print(D_00200000[0](1));
                test_print((int)D_00200000[0] - 0x110000);
                return 0;
            }
        """})
        self.assertEqual(out, ["2", "0"])

    def test_bitfields_unions_and_anonymous_members(self):
        out = self.run_program({"a.c": """
            typedef struct {
                unsigned lo : 4, hi : 4;
                union { int i; float f; };
                struct { short x, y; } pos;
            } Thing;
            extern Thing D_00200000;
            int test_main(void) {
                Thing *t = &D_00200000;
                t->lo = 3; t->hi = 9;
                t->f = 1.0f;
                t->pos.y = -2;
                test_print(*(unsigned char *)t);
                test_print(t->i);
                test_print(t->pos.y);
                test_print(sizeof(Thing));
                return 0;
            }
        """})
        self.assertEqual(out, ["147", "1065353216", "-2", "12"])

    def test_control_flow_strings_and_statics(self):
        out = self.run_program({"a.c": """
            int count(void) { static int n; int *p = &n; return ++*p; }
            int len(char *s) { int n = 0; while (*s++) n++; return n; }
            int test_main(void) {
                int i = 0;
                char *s = "ratchet";
                count(); count();
                test_print(count());
                test_print(len(s));
                test_print_str(s + 3);
                switch (len("ab")) { case 1: test_print(1); break; case 2: test_print(2); default: test_print(9); }
            again:
                i++;
                if (i < 3) goto again;
                do { i += 10; } while (i < 30);
                test_print(i);
                return 0;
            }
        """})
        self.assertEqual(out, ["3", "7", "chet", "2", "9", "33"])

    def test_quadword_helpers_and_64_bit_long(self):
        out = self.run_program({"a.c": """
            extern int D_00200000[4];
            extern int D_00200010[4];
            int test_main(void) {
                long v = 1;
                D_00200000[0] = 5; D_00200000[3] = 8;
                qcopy(D_00200010, D_00200000);
                test_print(D_00200010[0] + D_00200010[3]);
                v <<= 40;
                test_print((int)(v >> 38));
                test_print(sizeof(long));
                return 0;
            }
        """})
        self.assertEqual(out, ["13", "4", "8"])

    def test_call_before_declaration(self):
        # GCC 2.95 took a call made before the callee's declaration as int f().
        out = self.run_program({"a.c": """
            int test_main(void) { test_print(later(2)); return 0; }
            extern int later(int);
            int later(int x) { return x * 3; }
        """})
        self.assertEqual(out, ["6"])

    def test_functions_without_c_are_stubs(self):
        out = self.run_program({"a.c": """
            extern int func_00120000(int);
            int test_main(void) { test_print(func_00120000(1)); return 0; }
        """})
        self.assertEqual(out, ["missing", "func_00120000", "0"])


class TypeStrings(unittest.TestCase):
    def test_declarators(self):
        cases = {
            "int": "int x",
            "char *": "gaddr x",
            "S [4]": "S x[4]",
            "void (*)(int)": "gaddr x",
            "int (*[4])(void)": "gaddr x[4]",
            "float *[3][2]": "gaddr x[3][2]",
            "struct (unnamed struct at src/a.c:3:9)": "struct openrac_anon_src_a_c_3_9 x",
            "union S::(anonymous at a.c:1:25)": "union openrac_anon_a_c_1_25 x",
            "void (int) __attribute__((noreturn))": "void x(int)",
        }
        for text, want in cases.items():
            with self.subTest(text):
                self.assertEqual(ctype.declare(ctype.parse(text), "x"), want)

    def test_function_pointer_type(self):
        t = ctype.parse("int (float, int, ...)")
        self.assertEqual(ctype.fn_pointer(t), "int (*)(float, int, ...)")


class Preparation(unittest.TestCase):
    def test_long_is_64_bit(self):
        src = 'long a; unsigned long b; long long c; char *s = "long"; /* long */'
        self.assertEqual(prep.widen_long(src),
                         'long long a; unsigned long long b; long long c; char *s = "long"; /* long */')

    def test_file_scope_asm_is_blanked_keeping_lines(self):
        src = 'int a;\n__asm__(".section .text\n\tnop\n");\nint b;\nvoid f(void) { __asm__("sync"); }\n'
        out = prep.blank_file_scope_asm(src)
        self.assertEqual(out.count("\n"), src.count("\n"))
        self.assertNotIn("nop", out)
        self.assertIn('__asm__("sync")', out)


if __name__ == "__main__":
    unittest.main()
