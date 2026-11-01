/* Near miss: exact body at 0x0028FF58, 176 bytes, 28 of 28 places.
 *
 * The catalogue extent for func_0028FF50 starts eight bytes too early: the
 * first two words of the extent, `sw $v0,0x6C($a1)` and `nop`, are the
 * residue the previous body's compilation left behind (the store uses an
 * uninitialised $v0, exactly the pattern BOOT_RESIDUES records for
 * 0x00289018).  The callable body is 176 bytes at 0x0028FF58.
 *
 * Verified with `python scratch/cmpat.py src/game/0028F948.c func_0028FF50 8`:
 *   28 of 28 places match (skip 8)
 * With the catalogue extent as it stands, wslcmp reports
 *   func_0028FF50: 0 of 28 places match  [boot: 176 bytes, expected 184]
 *
 * Fix: add "func_0028FF50": (184, 8) to FUNCTION_ENTRY_OFFSETS in
 * tools/levels.py and regenerate the catalogue; the body below then matches.
 */
typedef struct {
    char pad00[8];
    int unk08;
    int *unk0C;
    char pad10[0x5C];
    int unk6C;
    unsigned char count[2];
    char pad72[2];
    int unk74;
    char pad78[4];
    int unk7C;
} AnimState;

void func_0028FF50(AnimState *a0)
{
    int *limit = a0->unk0C;
    unsigned char *count = a0->count;
    int value = a0->unk08;
    int top = *limit;

    a0->unk74 = top;
    if (value < top)
        a0->unk74 = value;
    else if (top < 0)
        a0->unk74 = 0;

    if (a0->unk7C >= 5) {
        a0->unk7C = 5;
        if (count[0] < 8) {
            count[0] = count[0] + 1;
            return;
        }
        if (count[1] < 8) {
            count[1] = count[1] + 1;
            return;
        }
    } else {
        a0->unk6C = 1;
        if (count[1] != 0) {
            count[1] = count[1] - 1;
            return;
        }
        if (count[0] != 0) {
            count[0] = count[0] - 1;
            return;
        }
        a0->unk6C = -6;
    }
}
