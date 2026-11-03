"""Explicit SDATA metadata controls; no retail data or compiler required."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import sdata_aliases


class ExplicitSdataAliases(unittest.TestCase):
    def test_ordinary_assembly_and_quoted_suffixes_are_byte_identical(self):
        text = ('.file 1 "ordinary__gp"\n# ignored__gp\n/* ignored2__gp */\n'
                '.extern ordinary, 14\n.ent function\nfunction:\n'
                ' lw $2,ordinary\n la $3,ordinary\n jr $31\n nop\n.end function\n')
        self.assertEqual(sdata_aliases.transform(text), text)

    def test_incomplete_array_reference_needs_no_extern_or_fabricated_owner_size(self):
        text = ('.file 1 "example.c"\ngcc2_compiled.:\n__gnu_compiled_c:\n'
                '.extern owner, 0\n.ent address\naddress:\n'
                ' la $2,owner__gp+13\n jr $31\n nop\n.end address\n')
        changed = sdata_aliases.transform(text)
        self.assertIn('.extern owner, 0\n', changed)
        self.assertIn('la $2,owner__gp+13\n', changed)
        self.assertIn('\towner__gp = owner\n', changed)
        self.assertLess(changed.index('__gnu_compiled_c:'), changed.index('.extern owner__gp, 1'))
        self.assertLess(changed.index('.extern owner__gp, 1'), changed.index('.ent address'))
        self.assertGreater(changed.index('owner__gp = owner'), changed.index('.end address'))
        self.assertEqual(sdata_aliases.transform(changed), changed)

    def test_only_alias_extern_metadata_changes(self):
        text = ('.extern owner, 28\n.extern owner__gp, 28 # address view\n'
                '.ent f\nf:\n lw $2,owner__gp+24\n lw $3,owner\n.end f\n')
        changed = sdata_aliases.transform(text)
        self.assertIn('.extern owner, 28\n', changed)
        self.assertIn('.extern owner__gp, 1 # address view\n', changed)
        before = [line for line in text.splitlines() if line.startswith((' lw', ' la'))]
        after = [line for line in changed.splitlines() if line.startswith((' lw', ' la'))]
        self.assertEqual(before, after)
        self.assertEqual(sdata_aliases.transform(changed), changed)

    def test_same_canonical_binding_is_respected(self):
        for binding in ('view__gp = view', '.equ view__gp,view', '.set view__gp,view'):
            with self.subTest(binding=binding):
                text = '.extern view__gp,1\n' + binding + '\n.ent f\nf:\n la $2,view__gp\n.end f\n'
                self.assertEqual(sdata_aliases.transform(text), text)

    def test_conflicting_storage_bindings_fail_closed(self):
        for text in ('x__gp = other\n', '.set x__gp,x+4\n', '.equ x__gp,other\n',
                     '.comm x__gp,14\n', '.lcomm x__gp,28\n', 'x__gp:\n',
                     '.extern x__gp,-1\n', '.extern x__gp__gp,0\n'):
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    sdata_aliases.transform(text)

    def test_all_aliases_share_only_their_own_canonical_storage(self):
        changed = sdata_aliases.transform('.ent f\nf:\n la $2,one__gp\n la $3,two__gp+27\n.end f\n')
        self.assertIn('one__gp = one\n', changed)
        self.assertIn('two__gp = two\n', changed)
        self.assertNotIn('one__gp = two', changed)
        self.assertEqual(sdata_aliases.transform(changed), changed)


if __name__ == "__main__":
    unittest.main()