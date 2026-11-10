import contextlib
import io
import tempfile
import unittest
import sys
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / 'tests'))
import asm
import asmc
import assembler
import ctype
from program import Function, Program, Signature, read_catalogue, prototype


class CatalogueTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('clang'), 'needs Clang')
    def test_marked_64_bit_result_survives_a_translated_call(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            wide = 'func_00100000'
            caller = 'func_00100100'
            ordinary = 'func_00100200'
            consumer = 'func_00100300'
            via_argument = 'func_00100400'
            value = ('dsll32 $2, $4, 0\nlui $3, 0x8765\nori $3, $3, 0x4321\n'
                     'dsll32 $3, $3, 0\ndsrl32 $3, $3, 0\nor $2, $2, $3\njr $31\nnop\n')
            bodies = {
                wide: (0x100000, value), ordinary: (0x100200, value),
                caller: (0x100100, 'addiu $29, $29, -16\nsd $31, 0($29)\n'
                    'jal func_00100000\nnop\ndsrl32 $2, $2, 0\n'
                    'ld $31, 0($29)\njr $31\naddiu $29, $29, 16\n'),
                consumer: (0x100300, 'dsrl32 $2, $4, 0\njr $31\nnop\n'),
                via_argument: (0x100400, 'addiu $29, $29, -16\nsd $31, 0($29)\n'
                    'jal func_00100000\nnop\ndaddu $4, $2, $0\n'
                    'jal func_00100300\nnop\nld $31, 0($29)\njr $31\naddiu $29, $29, 16\n'),
            }
            p = Program()
            p.returns_64 = {wide}
            for name, (address, body) in bodies.items():
                (folder / (name + '.s')).write_text(assembler.assemble(name, address, body,
                    {wide: 0x100000, consumer: 0x100300}))
                p.functions[name] = Function(name, Signature(ctype.Base('int'), (), False, False), asm=True)
            p.index_places()
            _, results = asmc.run(p, {'asm': {'enabled': True, 'dirs': ['.'], 'forward_unprototyped': True,
                'signatures': {consumer: 'int (unsigned long long)'}}}, folder)
            for result in results.values():
                self.assertIsNotNone(result.text, result.reason)
            asmc.write_files(folder, {'asm': {'enabled': True, 'files': 1}}, results, 'test')
            (folder / 'game_protos.h').write_text('#include "openrac/guest.h"\n' +
                '\n'.join(prototype(f) + ';' for f in p.functions.values()) + '\n')
            (folder / 'main.c').write_text('''
                #include "game_protos.h"
                static uint8_t ram[4096];
                uint8_t *openrac_guest_base = ram;
                gaddr openrac_guest_sp = sizeof ram;
                int main(void) {
                    if ((uint64_t)func_00100000(0x12345678) != UINT64_C(0x1234567887654321)) return 1;
                    if (func_00100100(0x12345678) != 0x12345678) return 2;
                    if ((uint32_t)func_00100200(0x12345678) != UINT32_C(0x87654321)) return 3;
                    if (func_00100400(0x12345678) != 0x12345678) return 4;
                    return openrac_guest_sp != sizeof ram;
                }
            ''')
            executable = folder / 'wide-result.exe'
            check = subprocess.run(['clang', '-std=gnu11',
                '-I', str(HERE.parent.parent / 'runtime/include'), str(folder / 'asm_0.c'),
                str(folder / 'main.c'), '-o', str(executable)], capture_output=True, text=True)
            self.assertEqual(check.returncode, 0, check.stderr)
            run = subprocess.run([str(executable)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)

    def test_verified_signature_holds_without_the_assembly(self):
        # A function whose C is left to its assembly (from_assembly) keeps its verified prototype
        # when there is nothing to translate (a build without the disc): the port's C that wraps
        # it is then compiled against one signature either way. A name only discovery adds is
        # passed over there.
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            name = 'func_00100000'
            p = Program()
            p.functions[name] = Function(name, Signature(ctype.Base('void'), (), True, False), asm=True)
            p.index_places()
            cfg = {'asm': {'enabled': True, 'dirs': ['.'], 'signatures': {
                name: 'int (int, void *)', 'func_00100100': 'int (int)'}}}
            stage, results = asmc.run(p, cfg, folder)
            self.assertEqual(results, {})
            self.assertEqual(prototype(p.functions[name]).split('(')[0].split()[0], 'int')
            self.assertEqual(len(p.functions[name].sig.params), 2)
            self.assertNotIn('func_00100100', p.functions)

    def test_signature_for_a_function_with_c_now_is_skipped(self):
        # The decomp matched a function after hostgen.json gave it an assembly signature: its C
        # gives the prototype, so the stale entry is passed over with a warning, not an error.
        name = 'func_00100000'
        p = Program()
        p.functions[name] = Function(name, Signature(ctype.Base('void'), (), True, False), unit='src/a.c')
        p.index_places()
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            asmc.apply_signatures(p, {'signatures': {name: 'int (int)'}}, discovered=True)
        self.assertEqual(p.functions[name].sig.ret, ctype.Base('void'))
        self.assertIn('has C now', err.getvalue())

    def test_catalogue_entry_gets_real_argument_signature(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            name = 'func_00100000'
            (folder / 'f.s').write_text(assembler.assemble(name, 0x100000,
                'addu $2, $4, $5\njr $31\nnop\n'))
            p = Program()
            f = Function(name, Signature(ctype.Base('int'), (), False, False), asm=True)
            p.functions[name] = f
            tr = asmc.Translator(p, asm.read_index([folder], lambda _: -1))
            tr.forward_unprototyped = True  # rac4's asm.forward_unprototyped
            tr.prepare_signature(f, tr.find(f))
            self.assertTrue(f.sig.prototyped)
            self.assertEqual(len(f.sig.params), 2)

    @unittest.skipUnless(shutil.which('clang'), 'needs Clang')
    def test_unprototyped_call_compiles_after_same_unit_definition(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            p = Program()
            sig = Signature(ctype.Base('int'), (), False, False)
            p.functions['func_00100000'] = Function('func_00100000', sig, asm=True, declared=[sig])
            p.functions['func_00100100'] = Function('func_00100100', sig, asm=True, declared=[sig])
            bodies = {
                'func_00100000': (0x100000, 'addu $2, $4, $0\njr $31\nnop\n'),
                'func_00100100': (0x100100, 'addiu $29, $29, -16\nsd $31, 0($29)\n'
                    'jal func_00100000\nnop\nld $31, 0($29)\njr $31\naddiu $29, $29, 16\n'),
            }
            for name, (addr, body) in bodies.items():
                (folder / (name + '.s')).write_text(assembler.assemble(name, addr, body,
                    {'func_00100000': 0x100000}))
            tr = asmc.Translator(p, asm.read_index([folder], lambda _: -1))
            tr.forward_unprototyped = True  # rac4's asm.forward_unprototyped
            for f in p.functions.values():
                tr.prepare_signature(f, tr.find(f))
                f.unit = '<asm>'
            p.index_places()
            results = {name: tr.translate(f) for name, f in p.functions.items()}
            for result in results.values():
                self.assertIsNotNone(result.text, result.reason)
            asmc.write_files(folder, {'asm': {'enabled': True, 'files': 1}}, results, 'test')
            (folder / 'game_protos.h').write_text('#include "openrac/guest.h"\n'
                'int func_00100000();\nint func_00100100();\n')
            check = subprocess.run(['clang', '-std=gnu11', '-fsyntax-only',
                '-I', str(HERE.parent.parent / 'runtime/include'),
                str(folder / 'asm_0.c')], capture_output=True, text=True)
            self.assertEqual(check.returncode, 0, check.stderr)

    def test_old_style_declaration_gets_inferred_parameters(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            name = 'func_00100000'
            (folder / 'f.s').write_text(assembler.assemble(name, 0x100000,
                'addu $2, $4, $5\njr $31\nnop\n'))
            p = Program()
            old = Signature(ctype.Base('int'), (), False, False)
            f = Function(name, old, asm=True, declared=[old])
            p.functions[name] = f
            tr = asmc.Translator(p, asm.read_index([folder], lambda _: -1))
            tr.forward_unprototyped = True  # rac4's asm.forward_unprototyped
            tr.prepare_signature(f, tr.find(f))
            self.assertTrue(f.sig.prototyped)
            self.assertEqual(len(f.sig.params), 2)

    def test_old_style_declaration_kept_by_default(self):
        # Without asm.forward_unprototyped (rac1, rac2) a compatible `int f()` stays as declared,
        # so the generated prototypes agree with the decompilation's own definitions.
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            name = 'func_00100000'
            (folder / 'f.s').write_text(assembler.assemble(name, 0x100000,
                'addu $2, $4, $5\njr $31\nnop\n'))
            p = Program()
            old = Signature(ctype.Base('int'), (), False, False)
            f = Function(name, old, asm=True, declared=[old])
            p.functions[name] = f
            tr = asmc.Translator(p, asm.read_index([folder], lambda _: -1))
            tr.prepare_signature(f, tr.find(f))
            self.assertFalse(f.sig.prototyped)
            self.assertEqual(f.sig.params, ())

    def test_forwarded_parameters_and_call_delay_slot(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            caller = 'func_00100000'
            callee = 'func_00100100'
            (folder / 'f.s').write_text(assembler.assemble(caller, 0x100000,
                'addiu $29, $29, -16\nsd $31, 0($29)\n'
                'jal func_00100100\naddiu $7, $0, 1\n'
                'ld $31, 0($29)\njr $31\naddiu $29, $29, 16\n',
                {callee: 0x100100}))
            p = Program()
            f = Function(caller, Signature(ctype.Base('int'), (), False, False), asm=True)
            p.functions[caller] = f
            p.functions[callee] = Function(callee,
                Signature(ctype.Base('int'), (ctype.Base('int'),) * 4, False, True))
            p.symbols[callee] = 0x100100
            p.index_places()
            tr = asmc.Translator(p, asm.read_index([folder], lambda _: -1))
            tr.forward_unprototyped = True  # rac4's asm.forward_unprototyped
            tr.prepare_signature(f, tr.find(f))
            self.assertEqual(len(f.sig.params), 3)
            result = tr.translate(f)
            self.assertIsNotNone(result.text, result.reason)
            self.assertIn('r4 =', result.text)

    def test_missing_entries_preserve_known_c_signatures(self):
        program = Program()
        known = Function('func_00100000', Signature(ctype.Base('float'), (), False, True), 'src/known.c')
        program.functions[known.symbol] = known
        with tempfile.TemporaryDirectory() as temp:
            table = Path(temp) / 'functions.tsv'
            table.write_text('# name\taddress\tsize\tsegment\n'
                             'func_00100000\t0x00100000\t0x20\tcore_text\n'
                             'func_00157C58\t0x00157C58\t0x340\tcore_text\n')
            self.assertEqual(read_catalogue(table, program), 1)
            self.assertEqual(read_catalogue(table, program), 0)
        self.assertIs(program.functions[known.symbol], known)
        missing = program.functions['func_00157C58']
        self.assertTrue(missing.asm)
        self.assertFalse(missing.sig.prototyped)
        self.assertIsNone(missing.unit)
        self.assertEqual(program.symbols['func_00157C58'], 0x157C58)


if __name__ == '__main__':
    unittest.main()
