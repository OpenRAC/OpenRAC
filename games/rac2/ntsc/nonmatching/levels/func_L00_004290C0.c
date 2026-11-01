/* func_L00_004290C0 (220 bytes, 12 places) - the arm structure is exact, the JOIN and the
 * byte-flag path are not.
 *
 * Measured with scratch/wslcmp.py: "00_aranos_tutorial: 208 bytes, expected 220" - three
 * words short, for five join spellings.
 *
 * What is exact (keep it):
 *  - the four `obj+0xC / +0x10 / +0x8 / +0x14` tests form a nested `>= 0` chain whose
 *    annulled `bltzl` slots carry the NEXT field's load; the fourth test is a plain
 *    `bltz` (not likely);
 *  - the table base is the POINTER stored at 0x001B1844 (lui $v1,0x1B ; lw $v1,0x1844),
 *    so the global must be declared as a pointer (`extern u8 *D_L00_001B1844;`) and not
 *    read through its address;
 *  - 0x00189EA0 and the 0x001FDA80 table must be SYMBOLS: as literals this cc1 emits
 *    `lui $a0,0x18 ; ori $a0,$a0,0x9EA0` instead of retail's `lui $a0,0x19 ;
 *    addiu $a0,$a0,-0x6160` (bit 15 of the low half is set).
 *
 * The residue, twelve bytes:
 *  - the JOIN. Retail keeps the shared `r != 0` normalisation in its own block after the
 *    arms (`beqz $v0` ; `addiu $v0,$zero,1` ; `b` ; `ld $ra`) and every arm branches
 *    there with a bare `b` + `nop`. This cc1 sinks the normalisation into each arm's
 *    delay slot as `sltu $v0,$zero,$v0` and branches straight to the epilogue. Five
 *    spellings were measured (`return r != 0;`, `if (r == 0) return 0; return 1;`, the
 *    mirrored pair, an `int out` local and a `goto` join): all give the same sink. What
 *    would be needed is a spelling whose comparison STAYS a branch (`beqz`), because a
 *    branch cannot be moved into a delay slot the way `sltu` is.
 *  - the byte-flag path (the else of the fourth test). Retail reads the two bytes and
 *    branches twice: `beqz` on +0x38 (delay slot `daddu $v0,$zero,$zero`, so it returns
 *    0), then `beqz` on +0x39 (delay slot `addiu $v0,$zero,0x1`, so it returns 1), then
 *    `daddu $v0,$zero,$zero` (returns 0). This cc1 folds the two tests into one
 *    `bnel` + `sltiu`, i.e. it returns (byte38 != 0). The three `return` statements
 *    below are the retail's semantics; the compiler rewrites them.
 */
typedef unsigned char u8;
typedef signed short s16;

extern int func_002ACBC8(void *a0);
extern int func_L00_003206B8(void *a0);
extern int func_002ACCA8(void *a0);
extern int func_002A8AF0(void *a0, void *a1, int a2);

extern u8 D_L00_00189EA0[];
extern u8 *D_L00_001FDA80[];
extern u8 *D_L00_001B1844;

int func_L00_004290C0(u8 *a0)
{
    u8 *p = *(u8 **)(D_L00_001B1844 + (*(s16 *)(a0 + 0x84) << 5) + 0x1C);
    int r;

    if (*(int *)(p + 0xC) >= 0) {
        r = func_002ACBC8(D_L00_00189EA0);
    } else if (*(int *)(p + 0x10) >= 0) {
        r = func_L00_003206B8(D_L00_00189EA0);
    } else if (*(int *)(p + 8) >= 0) {
        r = func_002ACCA8(D_L00_00189EA0);
    } else if (*(int *)(p + 0x14) >= 0) {
        u8 *e = D_L00_001FDA80[*(int *)(p + 0x14)];
        r = func_002A8AF0(D_L00_00189EA0, e + 0x10, *(int *)e);
    } else {
        if (*(u8 *)(p + 0x38) == 0) {
            return 0;
        }
        if (*(u8 *)(p + 0x39) == 0) {
            return 1;
        }
        return 0;
    }
    return r != 0;
}
