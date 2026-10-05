"""tools/port.py and mips.references on synthetic code and C only, never a disc:
python3 -m unittest discover -s tools"""

import unittest

import mips
import port
from test_mips import JR_RA, NOP, addiu, code, jal, lui, lw

A0, V0, S0, GP = 4, 2, 16, 28


def beq(rs: int, rt: int, offset: int) -> int:
    return 0x10000000 | rs << 21 | rt << 16 | offset & 0xFFFF


def beql(rs: int, rt: int, offset: int) -> int:
    return 0x50000000 | rs << 21 | rt << 16 | offset & 0xFFFF


class ReferenceTests(unittest.TestCase):
    def test_calls_pairs_and_gp(self):
        text = code(lui(V0, 0x0015), jal(0x00201000), addiu(A0, V0, 0x1234),       # the %lo half in the delay slot
                    lw(V0, GP, -0x7FF0), lui(S0, 0x0017), lw(V0, S0, -0x10), JR_RA, NOP)
        self.assertEqual(mips.references(text, 0x00300000, 0x00166D00),
                         [(1, "call", 0x00201000), (2, "data", 0x00151234), (3, "gp", 0x0015ED10), (5, "data", 0x0016FFF0)])
        self.assertEqual([r for r in mips.references(text, 0x00300000) if r[1] == "gp"], [])

    def test_a_top_half_ends_at_a_call_and_at_an_overwrite(self):
        text = code(lui(V0, 0x0015), jal(0x00201000), NOP, lw(A0, V0, 8),           # $v0 does not survive the call
                    lui(S0, 0x0015), addiu(S0, S0, 0x100), lw(A0, S0, 8), JR_RA, NOP)
        self.assertEqual([r for r in mips.references(text, 0x00300000) if r[1] == "data"], [(5, "data", 0x00150100)])

    def test_the_half_follows_the_branch_that_was_taken(self):
        # beql: its delay slot runs only when taken. The other path loads another top half and jumps away.
        text = code(beql(V0, 0, 4), lui(A0, 0x001E),        # 0, 1: taken -> 5 with $a0 = 0x1E0000
                    NOP, beq(0, 0, 3), lui(A0, 0x0016),     # 2..4: falls through, loads 0x16, always jumps to 7
                    jal(0x00201000), addiu(A0, A0, 0x3200),  # 5, 6: reached only from the beql
                    JR_RA, NOP)
        self.assertIn((6, "data", 0x001E3200), mips.references(text, 0x00300000))

    def test_padding_after_a_jump_passes_nothing_on(self):
        # bne over a dead nop: what the branch carries must reach its target intact.
        text = code(0x14400004, lui(S0, 0x0016),                 # 0, 1: bne $v0,$zero -> 5; the slot loads the half
                    beq(0, 0, 3), NOP,                            # 2, 3: b -> 6
                    NOP,                                          # 4: padding nothing reaches
                    NOP, addiu(A0, S0, -0x7F80), JR_RA, NOP)      # 5 falls into 6
        self.assertIn((6, "data", 0x00158080), mips.references(text, 0x00300000))

    def test_two_ways_in_with_different_halves_give_nothing(self):
        text = code(beq(V0, 0, 2), lui(A0, 0x001E), lui(A0, 0x0016), lw(V0, A0, 4), JR_RA, NOP)
        self.assertEqual(mips.references(text, 0x00300000), [])

    def test_two_copies_align(self):
        def copy(callee, table):
            return code(lui(V0, table >> 16), jal(callee), addiu(A0, V0, table & 0xFFFF), JR_RA, NOP)
        us, pal = mips.references(copy(0x00201000, 0x00150010), 0x00300000), mips.references(copy(0x00201400, 0x00150110), 0x00300800)
        self.assertEqual(port.align(us, pal), [("call", 0x00201000, 0x00201400, 1), ("data", 0x00150010, 0x00150110, 2)])
        with self.assertRaises(port.Skip):
            port.align(us, pal[:1])


SOURCE = '''# 1 "include/types.h" 1
typedef unsigned char u8;
typedef int s32;
# 3 "src/a.c" 2
typedef struct { s32 a; s32 b; } Pair;   /* a comment; with a semicolon */
struct Tagged { char *name; };
struct Later;
enum Mode { MODE_OFF, MODE_ON = 4 };
extern Pair D_00150010_a __asm__("D_00150010");
extern s32 D_0015ED10 __attribute__((sda)), D_0015ED14;
static int helper(int x) { return x + 1; }
void (*D_00150200)(int);
s32 FUN_00201000(Pair *, char *) __asm__("FUN_00201000");
void other(void) { helper(3); }
int FUN_00300000(void) {
    char *s = "D_0015ED14; }";
    return FUN_00201000(&D_00150010_a, s) + D_0015ED10 + MODE_ON;
}
'''


class SliceTests(unittest.TestCase):
    def test_blank_keeps_offsets(self):
        text = 'a = "x;}"; /* b; */ c = \'}\'; // d\ne'
        clean = port.blank(text)
        self.assertEqual(len(clean), len(text))
        self.assertEqual(clean.count(";"), 2)
        self.assertNotIn("}", clean)
        self.assertEqual(clean.count("\n"), 1)

    def test_items_and_what_they_declare(self):
        found = {(i["kind"], tuple(sorted(i["names"])), i["origin"]) for i in port.items(SOURCE)}
        self.assertIn(("typedef", ("u8",), "include/types.h"), found)
        self.assertIn(("typedef", ("Pair",), "src/a.c"), found)
        self.assertIn(("tag", ("Tagged",), "src/a.c"), found)
        self.assertIn(("tag", ("Later",), "src/a.c"), found)
        self.assertIn(("tag", ("MODE_OFF", "MODE_ON", "Mode"), "src/a.c"), found)
        self.assertIn(("decl", ("D_00150010_a",), "src/a.c"), found)
        self.assertIn(("decl", ("D_0015ED10", "D_0015ED14"), "src/a.c"), found)
        self.assertIn(("decl", ("D_00150200",), "src/a.c"), found)
        self.assertIn(("decl", ("FUN_00201000",), "src/a.c"), found)
        self.assertIn(("function", ("helper",), "src/a.c"), found)
        self.assertIn(("function", ("FUN_00300000",), "src/a.c"), found)

    def test_pieces_are_what_the_function_needs(self):
        chosen, target = port.pieces(SOURCE, "FUN_00300000")
        names = [tuple(sorted(c["names"])) for c in chosen]
        self.assertEqual(names, [("s32",), ("Pair",), ("MODE_OFF", "MODE_ON", "Mode"), ("D_00150010_a",),
                                 ("D_0015ED10", "D_0015ED14"), ("FUN_00201000",)])
        self.assertTrue(target["text"].startswith("int FUN_00300000"))
        self.assertEqual(target["about"], "")
        _, described = port.pieces(SOURCE + "/* Adds one\n * to x. */\nint more(int x) { return x + 1; }\n", "more")
        self.assertEqual(described["about"], "Adds one to x.")
        with self.assertRaises(port.Skip):
            port.pieces(SOURCE, "missing")

    def test_a_function_of_the_same_file_comes_as_a_prototype_a_static_one_whole(self):
        chosen, _ = port.pieces(SOURCE, "other")
        self.assertEqual([c["text"] for c in chosen], ["static int helper(int x) { return x + 1; }"])
        chosen, _ = port.pieces(SOURCE + "void caller(void) { other(); }\n", "caller")
        self.assertEqual([c["text"] for c in chosen], ["void other(void);"])

    def test_substitute_leaves_strings_and_comments(self):
        text = 'f(D_1, "D_1"); /* D_1 */ D_12;'
        self.assertEqual(port.substitute(text, port.blank(text), {"D_1": "X"}), 'f(X, "D_1"); /* D_1 */ D_12;')

    def test_link_name_and_object_parts(self):
        item = next(i for i in port.items(SOURCE) if "D_00150010_a" in i["names"])
        text = SOURCE[item["start"]:item["end"]]
        self.assertEqual(port.link_name(text), "D_00150010")
        self.assertEqual(port.object_parts(dict(item, clean=port.blank(text)), "D_00150010_a"), ("Pair", ""))
        array = {"clean": "extern s32 D_1[2] __attribute__((sda));"}
        self.assertEqual(port.object_parts(array, "D_1"), ("s32", "[2]"))
        several = {"clean": "extern char *D_1, D_2[4], **D_3;"}
        self.assertEqual([port.object_parts(several, n) for n in ("D_1", "D_2", "D_3")],
                         [("char *", ""), ("char", "[4]"), ("char **", "")])
        aggregate = {"clean": "extern struct { int a; } D_1, *D_2;"}
        self.assertEqual(port.object_parts(aggregate, "D_2"), ("struct { int a; } *", ""))


class NeverTests(unittest.TestCase):
    def test_a_folder_excludes_its_own_functions_not_the_ones_it_calls(self):
        import tempfile
        from pathlib import Path
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / "movie.c").write_text('extern int func_00118BC0(int);\n'
                                              'INCLUDE_ASM("asm/nonmatchings/text", func_0023CD60); /* a note */\n'
                                              'int func_0023B670(int x) { return func_00118BC0(x); }\n')
            self.assertEqual(port.functions_under(Path(tmp)), {"func_0023CD60", "func_0023B670"})


class ResolveTests(unittest.TestCase):
    def test_nearest_symbol_below_owns_a_reference(self):
        pairs = [("call", 0x201000, 0x201400), ("data", 0x150010, 0x150110), ("data", 0x150018, 0x150118),
                 ("gp", 0x15ED10, 0x15EE10), ("data", 0x1A0000, 0x1B0000)]
        found = port.resolve({"f": [0x201000], "pair": [0x150010], "small": [0x15ED10]}, pairs)
        self.assertEqual(found["f"][:2], (0x201400, {"call"}))
        self.assertEqual(found["pair"][:2], (0x150110, {"data"}))
        self.assertEqual(found["small"][0], 0x15EE10)       # the far reference above it (a string, a table) does not move it
        self.assertIn("gp", found["small"][1])

    def test_a_constant_far_above_is_not_a_reference(self):
        with self.assertRaises(port.Skip):                  # 0x20000000 | address: not a reference to the last symbol
            port.resolve({"client": [0x157F80]}, [("data", 0x20000000, 0x20000000)])

    def test_a_call_must_hit_the_symbol_itself(self):
        with self.assertRaises(port.Skip):
            port.resolve({"f": [0x201000]}, [("call", 0x201008, 0x201408)])

    def test_a_function_the_level_holds_twice(self):
        found = port.resolve({"f": [0x2865B0, 0x2865E0]}, [("call", 0x2865B0, 0x287600)])
        self.assertEqual(found["f"][:2], (0x287600, {"call"}))

    def test_the_assemblers_shape(self):
        words = mips.words(code(lui(V0, 0x16), lw(V0, V0, 0x10),            # lw $v0,SYM as the assembler expands it
                                lui(V0, 0x16), lw(A0, V0, 0x10),            # the compiler's split: another register
                                lui(1, 0x16), 0xAC220010))                  # sw $v0,SYM: through $at
        self.assertEqual([port.through_macro(words, i) for i in (1, 3, 5)], [True, False, True])

    def test_names_in_the_target(self):
        pal = {"at": {(5, 0x240968): "func_L05_00240968", (5, 0x22DEB0): "func_001F9BC0"},
               "functions": {"func_001F3D78"}, "level_data_from": "0x15F000"}
        self.assertEqual(port.target_name(pal, 0x22DEB0, 5, True), "func_001F9BC0")       # the level's copy of an executable function
        self.assertEqual(port.target_name(pal, 0x1252C0, 5, True), "func_001252C0")       # resident code keeps its address
        self.assertIsNone(port.target_name(pal, 0x250000, 5, True))                       # level code the catalogue does not list
        self.assertEqual(port.target_name(pal, 0x13F450, 5, False), "D_0013F450")
        self.assertEqual(port.target_name(pal, 0x174340, 5, False), "D_L05_00174340")
        self.assertEqual(port.target_name(pal, 0x1F3D78, None, False), "func_001F3D78")   # a function's address taken as data
        self.assertEqual(port.target_name(pal, 0x174340, None, False), "D_00174340")


US_C = '''# 1 "games/x/us/include/types.h" 1
typedef unsigned char u8; typedef int s32; typedef float f32; typedef s32 b32;
# 1 "games/x/us/src/a.c" 2
typedef struct { s32 a; } Pair;
extern Pair D_00150010_a __asm__("D_00150010");
extern s32 D_0015EC10 __attribute__((sda));
extern u8 D_0016FFF0;
extern s32 D_00180000;
b32 FUN_00201000(Pair *);
/* Sums three globals. */
long long FUN_00300000(void) {
    return FUN_00201000(&D_00150010_a) + D_0015EC10 + D_0016FFF0 + D_00180000;
}
'''


def copy_of(callee: int, pair: int, gp_offset: int, byte: int, word: int) -> bytes:
    """One function in two versions: a call with a table address, a $gp load, a split byte load,
    and a word loaded the way the assembler expands `lw $v1,SYM`."""
    lbu = 0x90000000 | S0 << 21 | V0 << 16 | byte & 0xFFFF
    return code(lui(V0, pair >> 16), jal(callee), addiu(A0, V0, pair & 0xFFFF), lw(V0, GP, gp_offset),
                lui(S0, (byte + 0x8000) >> 16), lbu, lui(3, word >> 16), lw(3, 3, word & 0xFFFF), JR_RA, NOP)


class PortTests(unittest.TestCase):
    """One function carried from a made-up US version to a made-up PAL one."""

    def setUp(self):
        us = copy_of(0x00201000, 0x00150010, -0x7FF0, 0x0016FFF0, 0x00180000)
        pal = copy_of(0x00201400, 0x00150110, -0x7FF0, 0x001700F0, 0x00180400)
        self.src = {"key": "x/us", "root": "games/x/us", "gp": 0x166C00, "include": [], "places": {}, "symbols": {},
                    "shared_headers": ["types.h"], "types": {"b32": "s32"}, "phrases": [(r"\blong\s+long\b", "long")],
                    "credit": "Adapted from US: {file}, {name}.", "programs": {"boot": None}, "loaded": {"boot": {".text": (0x00300000, us)}}}
        self.dst = {"key": "x/pal", "gp": 0x166D00, "at": {}, "functions": {"func_00300800"}, "level_data_from": "0x15F000",
                    "programs": {"boot": None}, "loaded": {"boot": {".text": (0x00300800, pal)}}}
        self.row = {"kind": "same", "size": str(len(us)), "from_name": "FUN_00300000", "from_program": "boot",
                    "from_address": "00300000", "to_program": "boot", "to_address": "00300800"}
        self.saved = port.definition_file, port.preprocess
        port.definition_file = lambda src, name: port.ROOT / "games/x/us/src/a.c"
        port.preprocess = lambda path, include, defines: US_C

    def tearDown(self):
        port.definition_file, port.preprocess = self.saved

    def test_every_symbol_gets_the_targets_name_and_form(self):
        name, text = port.port(self.row, self.src, self.dst, {})
        self.assertEqual(name, "func_00300800")
        self.assertEqual(text, '''typedef struct { s32 a; } Pair;
extern Pair D_00150110;
extern short D_0015ED10;
extern u8 D_001700F0 NOT_SDA;
extern s32 D_00180400 MACRO_ADDR;
s32 func_00201400(Pair *);

/* Sums three globals.
   Adapted from US: src/a.c, FUN_00300000. */
long func_00300800(void) {
    return func_00201400(&D_00150110) + (*(s32 *)&D_0015ED10) + D_001700F0 + D_00180400;
}
''')

    def test_hints_keep_the_candidate_out_of_its_files_way(self):
        _, text = port.port(self.row, self.src, self.dst, {"alias": ["D_00150110", "func_00201400"], "rename": ["Pair"]})
        self.assertIn("typedef struct { s32 a; } Pair_00800;", text)
        self.assertIn('extern Pair_00800 D_00150110_00800 __asm__("D_00150110");', text)
        self.assertIn('s32 func_00201400_00800(Pair_00800 *) __asm__("func_00201400");', text)
        self.assertIn("func_00201400_00800(&D_00150110_00800)", text)

    def test_another_targets_own_rules(self):
        # A target that declares everything under private names, marks small data with an attribute and names
        # functions its own way (rac1/ntsc); the other reading of MACRO_ADDR comes from a hint.
        theirs = dict(self.dst, alias="always", small_data="sda", names={"function": "FUN_{addr:08x}"},
                      headers={"MACRO_ADDR": "sda.h"}, functions={"FUN_00300800"})
        name, text = port.port(self.row, self.src, theirs, {})
        self.assertEqual(name, "FUN_00300800")
        self.assertTrue(text.startswith('#include "sda.h"\ntypedef struct { s32 a; } Pair;\n'))
        for line in ('extern Pair D_00150110_00800 __asm__("D_00150110");',
                     'extern s32 D_0015ED10_00800 __asm__("D_0015ED10") __attribute__((sda));',
                     'extern u8 D_001700F0_00800 __asm__("D_001700F0");',
                     'extern s32 D_00180400_00800 __asm__("D_00180400") MACRO_ADDR;',
                     's32 FUN_00201400_00800(Pair *) __asm__("FUN_00201400");',
                     "long FUN_00300800(void) {",
                     "    return FUN_00201400_00800(&D_00150110_00800) + D_0015ED10_00800 + D_001700F0_00800 + D_00180400_00800;"):
            self.assertIn(line, text)
        _, other = port.port(self.row, self.src, theirs, {"macro_addr": "mixed"})
        self.assertIn('extern s32 D_00180400_00800 __asm__("D_00180400");', other)
        self.assertNotIn("sda.h", other)

    def test_a_place_the_target_counts_as_a_copy(self):
        theirs = dict(self.dst, functions={"FUN_L03_00300400"}, at={(3, 0x00300800): "FUN_L03_00300400"},
                      programs={"level:03": None}, loaded={"level:03": self.dst["loaded"]["boot"]})
        row = dict(self.row, to_program="level:03")
        with self.assertRaises(port.Skip) as why:
            port.port(row, self.src, theirs, {})
        self.assertIn("copy of FUN_L03_00300400", str(why.exception))

    def test_headers_that_do_not_travel(self):
        self.src["refuse_headers"] = ["a.c"]
        with self.assertRaises(port.Skip):
            port.port(self.row, self.src, self.dst, {})

    def test_what_a_candidate_may_not_contain(self):
        port.preprocess = lambda path, include, defines: US_C.replace("return", '__asm__ __volatile__("sq $0,0($4)"); return')
        with self.assertRaises(port.Skip):
            port.port(self.row, self.src, self.dst, {})

    def test_the_code_must_reach_every_symbol(self):
        port.preprocess = lambda path, include, defines: US_C.replace("D_00180000", "D_00140000")
        with self.assertRaises(port.Skip):
            port.port(self.row, self.src, self.dst, {})


if __name__ == "__main__":
    unittest.main()
