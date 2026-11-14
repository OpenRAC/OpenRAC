"""groups.py: the pvar words that hold a moby group, read from the game's code."""

import shutil
import tempfile
import unittest
from pathlib import Path

import groups

C = """typedef unsigned int gaddr;
extern char *openrac_guest_base;
#define G(a) ((void*)(openrac_guest_base + (gaddr)(a)))
#define GREF(T, a) (*(T*)G(a))
struct Moby { char pad[0x78]; gaddr pvars; };
typedef struct { char pad[0x20]; int zone; short group; } TurretVars;
int game_FUN_L01_0026e008(int i)
{
    gaddr entry = 0;
    int n = 0;
    (entry = (gaddr)(GREF(gaddr, ((gaddr)(0x001ABCC0u) + (gaddr)(i) * (gaddr)sizeof(gaddr)))));
    do
    {
        n += GREF(unsigned short, entry) & 32767;
    }
    while (GREF(short, ((entry += 2) - 2)) >= 0);
    return n;
}
void game_FUN_L17_002cb310(gaddr moby)
{
    gaddr vars = (gaddr)((((struct Moby *)G(moby))->pvars));
    game_FUN_L01_0026e008((int)((((TurretVars *)G(vars))->zone)));
    game_FUN_L01_0026e008((int)(GREF(short, (((gaddr)(vars) + (gaddr)(36) * (gaddr)sizeof(char))))));
}
"""


@unittest.skipUnless(shutil.which("clang"), "clang measures the words")
class Groups(unittest.TestCase):
    def test_a_group_number_passed_from_the_pvars_is_found(self):
        with tempfile.TemporaryDirectory() as folder:
            gen = Path(folder) / "gen"
            gen.mkdir()
            (gen / "turret.c").write_text(C)
            libraries = Path(folder) / "libraries.tsv"
            libraries.write_text("FUN_L17_002cb310\tmoby\tTurret\thook\tThe update of class turret (347), each tick\n")
            out = Path(folder) / "fields.tsv"
            self.assertEqual(groups.write(gen, libraries, out), 1)
            self.assertEqual(groups.table(out), {(347, 17): [(32, 4), (36, 2)]})
            self.assertEqual(groups.fields(347, 17, groups.table(out)), [(32, 4), (36, 2)])


if __name__ == "__main__":
    unittest.main()
