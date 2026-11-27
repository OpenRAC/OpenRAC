# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Structural rewrites on synthetic C; no game/compiler toolchain is required.

Run with ``python3 -m unittest discover -s tools``. Parser tests use either
decomp-permuter's parser or pycparser when installed. Behavioral tests additionally
need a native host gcc: they compile and execute both versions, rather than
assuming that an AST edit is semantics preserving.
"""
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from permuter import structural

try:
    from perm_pycparser import c_ast as ca, c_generator, c_parser
except ModuleNotFoundError as error:
    if error.name != "perm_pycparser":
        raise
    try:
        from pycparser import c_ast as ca, c_generator, c_parser
    except ModuleNotFoundError as error:
        if error.name != "pycparser":
            raise
        ca = c_generator = c_parser = None


def nodes(node):
    yield node
    for _name, child in node.children():
        yield from nodes(child)


class Region:
    """The upstream contract: a region must contain the entire selected node."""
    def __init__(self, *roots):
        self.allowed = {id(n) for root in roots for n in nodes(root)}

    def contains_node(self, node):
        return all(id(n) in self.allowed for n in nodes(node))


class StructuralHelpers:
    def parse(self, source):
        return c_parser.CParser().parse(source)

    def render(self, ast):
        return c_generator.CGenerator().visit(ast)

    def function(self, ast, name="target"):
        return next(n for n in ast.ext if isinstance(n, ca.FuncDef) and n.decl.name == name)

    def offered(self, source, *enabled):
        ast = self.parse(source)
        return ast, structural.plans(ast, "target", ca, enabled)

    def assert_no_plan(self, source, name):
        _ast, offered = self.offered(source, name)
        self.assertEqual(offered, [], source)


@unittest.skipUnless(ca is not None, "pycparser or decomp-permuter's parser is required")
class StructuralTest(StructuralHelpers, unittest.TestCase):
    def test_enumeration_does_not_edit_the_ast(self):
        ast = self.parse("int target(int n) { int a; int b; if (n) return 1; return 2; }")
        before = self.render(ast)
        offered = structural.plans(ast, "target", ca, structural.PASSES)
        self.assertTrue(offered)
        self.assertEqual(self.render(ast), before)

    def test_only_explicitly_enabled_passes_are_offered(self):
        ast = self.parse("int target(int n) { int a; int b; if (n) return 1; return 2; }")
        offered = structural.plans(ast, "target", ca, ["guard_return"])
        self.assertTrue(offered)
        self.assertEqual({p.name for p in offered}, {"guard_return"})
        self.assertEqual(structural.plans(ast, "target", ca, []), [])
        with self.assertRaisesRegex(ValueError, "unknown structural pass"):
            structural.plans(ast, "target", ca, ["reverse_cond_typo"])

    def test_assigned_function_must_be_unique(self):
        ast = self.parse("int other(void) { return 1; }")
        with self.assertRaisesRegex(ValueError, "assigned function"):
            structural.plans(ast, "target", ca, structural.PASSES)
        ast = self.parse("int target(void) { return 1; } int target(void) { return 2; }")
        with self.assertRaisesRegex(ValueError, "assigned function"):
            structural.plans(ast, "target", ca, structural.PASSES)

    def test_each_pass_has_a_finite_empty_result(self):
        for name in structural.PASSES:
            with self.subTest(name=name):
                self.assert_no_plan("int target(int x) { return x; }", name)

    def test_plans_never_edit_another_function_or_global(self):
        samples = {
            "reverse_cond": "if (n) return 1; else return 2;",
            "reorder_uninitialized_decls": "int a; int b; a = n; b = 2; return a+b;",
            "guard_return": "if (n) return 1; return 2;",
            "while_to_guarded_do": "while (n > 0) { --n; } return n;",
        }
        for name, body in samples.items():
            with self.subTest(name=name):
                ast = self.parse("int global = 7; int other(int n) { " + body +
                                 " } int target(int n) { " + body + " }")
                other = self.render(self.function(ast, "other"))
                global_decl = self.render(ast.ext[0])
                offered = structural.plans(ast, "target", ca, [name])
                self.assertTrue(offered)
                offered[0].apply()
                self.assertEqual(self.render(self.function(ast, "other")), other)
                self.assertEqual(self.render(ast.ext[0]), global_decl)

    def test_reverse_region_selects_only_the_contained_if(self):
        ast = self.parse("int target(int n) { if (n) n=4; else n=7; "
                         "{ if (n > 2) n+=1; else n-=1; } return n; }")
        outer, inner = self.function(ast).body.block_items[:2]
        before = self.render(outer)
        offered = structural.plans(ast, "target", ca, ["reverse_cond"], Region(inner))
        self.assertEqual(len(offered), 1)
        offered[0].apply()
        self.assertEqual(self.render(outer), before)

    def test_a_condition_only_region_cannot_swap_whole_arms(self):
        ast = self.parse("int target(int n) { if (n) return 1; else return 2; }")
        branch = self.function(ast).body.block_items[0]
        self.assertEqual(structural.plans(ast, "target", ca, ["reverse_cond"],
                                         Region(branch.cond)), [])

    def test_reorder_requires_both_declarations_in_the_region(self):
        ast = self.parse("int target(void) { int a; int b; int c; return 0; }")
        a, b, _c = self.function(ast).body.block_items[:3]
        self.assertEqual(structural.plans(ast, "target", ca, ["reorder_uninitialized_decls"],
                                         Region(a)), [])
        offered = structural.plans(ast, "target", ca, ["reorder_uninitialized_decls"], Region(a, b))
        self.assertEqual(len(offered), 1)

    def test_guard_requires_the_moved_suffix_to_be_inside_the_region(self):
        ast = self.parse("int target(int n) { if (n) return 1; return 2; }")
        branch, suffix = self.function(ast).body.block_items
        self.assertEqual(structural.plans(ast, "target", ca, ["guard_return"], Region(branch)), [])
        self.assertEqual(len(structural.plans(ast, "target", ca, ["guard_return"],
                                             Region(branch, suffix))), 1)

    def test_loop_requires_the_entire_while_in_the_region(self):
        ast = self.parse("int target(int n) { while (n > 0) { --n; } return n; }")
        loop = self.function(ast).body.block_items[0]
        self.assertEqual(structural.plans(ast, "target", ca, ["while_to_guarded_do"], Region(loop.stmt)), [])
        self.assertEqual(len(structural.plans(ast, "target", ca, ["while_to_guarded_do"], Region(loop))), 1)

    def test_declarations_with_initializers_or_intervening_effects_are_not_reordered(self):
        for body in ("int a=effect(); int b;", "int a; int b=effect();",
                     "int a; effect(); int b;", "static int a; int b;",
                     "volatile int a; int b;"):
            with self.subTest(body=body):
                self.assert_no_plan("int effect(void); int target(void) { " + body + " return 0; }",
                                    "reorder_uninitialized_decls")

    def test_arrays_variable_types_and_function_declarators_are_not_reordered(self):
        for body in ("int a[4]; int b;", "int a[n++]; int b;",
                     "int (*p)[n++]; int b;", "int (*cb)(void); int b;"):
            with self.subTest(body=body):
                self.assert_no_plan("int target(int n) { " + body + " return n; }",
                                    "reorder_uninitialized_decls")

    def test_inline_tag_definitions_do_not_rebind_preceding_declarations(self):
        samples = (
            "struct S { int a; }; int target(void) { struct S *p; "
            "struct S { long long a; } *q; return sizeof(*p); }",
            "union U { char a; }; int target(void) { union U *p; "
            "union U { double a; } *q; return sizeof(*p); }",
            "int target(void) { int n; enum { N=sizeof n } *p; return N; }",
        )
        for source in samples:
            with self.subTest(source=source):
                self.assert_no_plan(source, "reorder_uninitialized_decls")

    def test_address_exposed_objects_are_not_reordered_even_with_shadowed_names(self):
        samples = (
            "int a; int b; expose(&a); a=1; b=2; return a+b;",
            "int a; int b; expose(&b); a=1; b=2; return a+b;",
            "int a; int b; { int a; expose(&a); } a=1; b=2; return a+b;",
        )
        for body in samples:
            with self.subTest(body=body):
                self.assert_no_plan("void expose(int *); int target(void) { " + body + " }",
                                    "reorder_uninitialized_decls")

    def test_unary_not_can_be_removed_without_also_changing_its_expression(self):
        ast, offered = self.offered("int target(int n) { if (!n) return 1; else return 2; }", "reverse_cond")
        self.assertEqual(len(offered), 1)
        offered[0].apply()
        branch = self.function(ast).body.block_items[0]
        self.assertIsInstance(branch.cond, ca.ID)

    def test_guard_declines_else_and_nonterminal_suffixes(self):
        self.assert_no_plan("int target(int n) { if (n) return 1; else return 2; }", "guard_return")
        self.assert_no_plan("int target(int n) { if (n) return 1; n=2; }", "guard_return")

    def test_void_guard_cannot_return_early_from_a_nested_scope(self):
        self.assert_no_plan("int value; void target(int n) { { if (n) value=1; } value+=10; }", "guard_return")

    def test_guard_does_not_lose_tags_or_enumerators_declared_in_its_condition(self):
        sources = (
            "enum { K=3 }; int target(int n) { if(sizeof(enum { K=7 })==n) return K; return 2; }",
            "enum { K=3 }; int value; void target(int n) { if(sizeof(enum { K=7 })==n) value=K; }",
            "struct S { int x; }; int target(int n) { "
            "if(sizeof(struct S { long long x; })==n) return sizeof(struct S); return 2; }",
        )
        for source in sources:
            with self.subTest(source=source):
                self.assert_no_plan(source, "guard_return")

    def test_guard_does_not_move_labels_gotos_breaks_or_continues(self):
        samples = (
            "if (n) { label: return 1; } return 2;",
            "if (n) return 1; goto done; done: return 2;",
            "while (n) { if (n==2) { break; return 1; } return 2; } return 3;",
            "while (n) { if (n==2) { continue; return 1; } return 2; } return 3;",
        )
        for body in samples:
            with self.subTest(body=body):
                self.assert_no_plan("int target(int n) { " + body + " }", "guard_return")

    def test_loop_declines_effectful_or_variable_type_conditions(self):
        for cond in ("probe()", "n-- > 0", "(n=n-1)", "(n++, n<5)",
                     "sizeof(int[n++]) > 0"):
            with self.subTest(cond=cond):
                self.assert_no_plan("int probe(void); int target(int n) { while (" + cond +
                                    ") { --n; } return n; }", "while_to_guarded_do")

    def test_loop_declines_labels_gotos_and_switch_entry_points(self):
        for body in ("label: --n;", "if (n) goto done; --n;",
                     "switch(n) { case 1: --n; break; default: --n; }"):
            with self.subTest(body=body):
                self.assert_no_plan("int target(int n) { while (n>0) { " + body +
                                    " } done: return n; }", "while_to_guarded_do")


@unittest.skipUnless(ca is not None, "pycparser or decomp-permuter's parser is required")
@unittest.skipUnless(shutil.which("gcc"), "native host gcc is required for executable C comparisons")
class CompiledStructuralTest(StructuralHelpers, unittest.TestCase):
    """Execution comparisons include zero-trip loops, early exits and side effects."""
    def run_c(self, source, driver, optimization):
        with tempfile.TemporaryDirectory(prefix="openrac-structural-") as directory:
            work = Path(directory)
            path = work / "test.c"
            program = work / ("test.exe" if os.name == "nt" else "test")
            text = "#include <stdio.h>\n#include <math.h>\n" + source + "\n" + driver
            path.write_text(text)
            command = [shutil.which("gcc"), "-std=c11", optimization, "-fno-fast-math", str(path), "-o", str(program)]
            built = subprocess.run(command, capture_output=True, text=True, timeout=30)
            self.assertEqual(built.returncode, 0, built.stderr + "\n" + text)
            ran = subprocess.run([str(program)], capture_output=True, text=True, timeout=5)
            self.assertEqual(ran.returncode, 0, ran.stderr + "\n" + text)
            return ran.stdout

    def assert_rewrites_preserve(self, source, name, driver, expected=None):
        ast, offered = self.offered(source, name)
        self.assertTrue(offered, "the positive fixture must exercise a real plan")
        original = self.render(ast)
        alternatives = []
        for index in range(len(offered)):
            ast, fresh = self.offered(source, name)
            fresh[index].apply()
            changed = self.render(ast)
            self.assertNotEqual(original, changed, "the offered plan must actually edit C")
            alternatives.append(changed)
        for optimization in ("-O0", "-O2"):
            baseline = self.run_c(original, driver, optimization)
            if expected is not None:
                self.assertEqual(baseline, expected)
            for index, changed in enumerate(alternatives):
                with self.subTest(pass_name=name, plan=index, optimization=optimization):
                    self.assertEqual(self.run_c(changed, driver, optimization), baseline)

    def test_reverse_if_preserves_nan_short_circuit_and_single_evaluation(self):
        source = """
            int calls; int arms; int rhs_calls;
            int rhs(void) { ++rhs_calls; return 1; }
            int target(double x) {
                if ((calls++, x < 0.0) && rhs()) { arms+=3; return 11; }
                else { arms+=7; return 22; }
            }
        """
        driver = """
            int main(void) {
                double values[] = { NAN, INFINITY, -INFINITY, -0.0, 0.0, 1.0, -1.0 };
                for (int i=0; i<7; ++i) {
                    calls=0; arms=0; rhs_calls=0; int answer=target(values[i]);
                    printf("%d:%d:%d:%d\\n", answer,calls,arms,rhs_calls);
                }
                return 0;
            }
        """
        self.assert_rewrites_preserve(source, "reverse_cond", driver,
            "22:1:7:0\n22:1:7:0\n11:1:3:1\n22:1:7:0\n22:1:7:0\n22:1:7:0\n11:1:3:1\n")

    def test_reverse_ternary_preserves_nan_and_only_the_selected_arm(self):
        source = "int calls; int arms; int target(double x) { " \
                 "return (calls++, x!=x) ? (arms+=3,11) : (arms+=7,22); }"
        driver = "int main(void) { double xs[]={NAN,0.0,-0.0,-1.0}; " \
                 "for(int i=0;i<4;++i){calls=0;arms=0;int r=target(xs[i]);" \
                 'printf("%d:%d:%d\\n",r,calls,arms);} return 0; }'
        self.assert_rewrites_preserve(source, "reverse_cond", driver,
                                      "11:1:3\n22:1:7\n22:1:7\n22:1:7\n")

    def test_reorder_fixed_objects_preserves_assignments_and_pointer_reads(self):
        source = "int values[1]; int target(int n) { int a; int *p; int b; " \
                 "a=n+3; p=values; b=a*2+*p; return b+1; }"
        driver = 'int main(void){printf("%d,%d,%d\\n",target(0),target(4),target(-2));return 0;}'
        self.assert_rewrites_preserve(source, "reorder_uninitialized_decls", driver, "7,15,3\n")

    def test_existing_struct_pointer_remains_eligible(self):
        source = "struct S { int a; }; int target(int n) { struct S *p; int x; x=n+1; return sizeof(*p)+x; }"
        driver = 'int main(void){printf("%d,%d\\n",target(0),target(4));return 0;}'
        self.assert_rewrites_preserve(source, "reorder_uninitialized_decls", driver)

    def test_inline_tag_scope_rejection_has_an_executable_counterexample(self):
        before = "struct S { int a; }; int target(void) { struct S *p; " \
                 "struct S { long long a; } *q; return sizeof(*p); }"
        unsafe_order = "struct S { int a; }; int target(void) { " \
                       "struct S { long long a; } *q; struct S *p; return sizeof(*p); }"
        self.assert_no_plan(before, "reorder_uninitialized_decls")
        driver = 'int main(void){printf("%d\\n",target());return 0;}'
        for optimization in ("-O0", "-O2"):
            original = self.run_c(before, driver, optimization)
            unsafe = self.run_c(unsafe_order, driver, optimization)
            # This is a negative control, not an accepted rewrite. sizeof never
            # reads either uninitialized pointer; the changed result is binding.
            self.assertNotEqual(original, unsafe)

    def test_guard_return_preserves_condition_calls_effect_order_and_scope(self):
        source = """
            int checks; int trace;
            int probe(int x) { ++checks; return x; }
            int target(int n) {
                int value=10; trace=3;
                if (probe(n)) { int value=11; trace=trace*10+1; return value; }
                int result=value+2; trace=trace*10+2;
                return result;
            }
        """
        driver = "int main(void){for(int n=0;n<3;++n){checks=0;trace=0;int r=target(n);" \
                 'printf("%d:%d:%d\\n",r,checks,trace);}return 0;}'
        self.assert_rewrites_preserve(source, "guard_return", driver,
                                      "12:1:32\n11:1:31\n11:1:31\n")

    def test_guard_void_preserves_nested_returns_and_false_fallthrough(self):
        source = "int trace; void target(int n) { if(n){trace=10;if(n==3)return;trace+=5;} }"
        driver = "int main(void){int xs[]={0,1,3};for(int i=0;i<3;++i){trace=-1;target(xs[i]);" \
                 'printf("%d\\n",trace);}return 0;}'
        self.assert_rewrites_preserve(source, "guard_return", driver, "-1\n15\n10\n")

    def test_guard_condition_scope_rejection_has_an_executable_counterexample(self):
        examples = (
            ("enum { K=3 }; int target(int n) { "
             "if(sizeof(enum { K=7 })==n) return K; return 2; }",
             "enum { K=3 }; int target(int n) { "
             "if(!(sizeof(enum { K=7 })==n)) { return 2; } return K; }",
             'int main(void){printf("%d\\n",target(sizeof(enum { ARG=7 })));return 0;}'),
            ("enum { K=3 }; int value; void target(int n) { "
             "if(sizeof(enum { K=7 })==n) value=K; }",
             "enum { K=3 }; int value; void target(int n) { "
             "if(!(sizeof(enum { K=7 })==n)) return; value=K; }",
             'int main(void){value=0;target(sizeof(enum { ARG=7 }));printf("%d\\n",value);return 0;}'),
        )
        for before, unsafe_order, driver in examples:
            self.assert_no_plan(before, "guard_return")
            for optimization in ("-O0", "-O2"):
                original = self.run_c(before, driver, optimization)
                unsafe = self.run_c(unsafe_order, driver, optimization)
                self.assertEqual(original, "7\n")
                self.assertEqual(unsafe, "3\n")

    def test_guarded_do_preserves_zero_trip_break_continue_and_body_scope(self):
        source = """
            int steps; int trace;
            int target(int n) {
                int *outer=&n;
                while(n>0) {
                    int n=100; ++steps; trace=trace*3+1;
                    --*outer;
                    if(*outer==2) continue;
                    if(*outer==1) break;
                    trace+=n;
                }
                return n;
            }
        """
        driver = "int main(void){for(int n=-1;n<8;++n){steps=0;trace=0;int r=target(n);" \
                 'printf("%d:%d:%d\\n",r,steps,trace);}return 0;}'
        self.assert_rewrites_preserve(source, "while_to_guarded_do", driver)

    def test_guarded_do_preserves_nested_loop_targets_and_early_return(self):
        source = """
            int trace;
            int target(int n) {
                while(n>0) {
                    --n;
                    if(n==4) return trace+77;
                    int j=3;
                    while(j>0) { --j; if(j==1)continue; if(j==0)break; trace+=j; }
                    trace+=n;
                }
                return trace;
            }
        """
        driver = "int main(void){for(int n=0;n<8;++n){trace=0;int r=target(n);" \
                 'printf("%d:%d\\n",r,trace);}return 0;}'
        self.assert_rewrites_preserve(source, "while_to_guarded_do", driver)


if __name__ == "__main__":
    unittest.main()
