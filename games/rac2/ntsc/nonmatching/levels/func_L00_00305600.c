/* func_L00_00305600 - NON MATCHEE (wave 8, agent w8-09).
 *
 * The function is understood: it scans the 0x30-entry gadget-class table of
 * contents at stride 0x14, returns 1 when the object is absent, and on a hit
 * returns 0 unless the small-data slot word holds -1, in which case it loads
 * the class (func_002FC3C0(1), then func_002946C0(object, *(short *)0x152CE4,
 * argument)) and returns 1.  The `!= -1` test reads the small-data word, not
 * the index: the bne's delay slot loads it, which is why the register differs
 * from the loop index at that point.
 *
 * Corrected symbol name (the previous draft had it wrong, so its relocations
 * could never check out): the table base is 0x14FB540, not 0x14B540.  Retail
 * materialises it with `lui $v0,0x15 ; addiu $a0,$v0,-0x4AC0` and reads the
 * first entry with `lw $v0,0x4B40($a0)`, so the entries sit 0x4B40 into the
 * object and the object is 0x1500080 when the entries start.  The other two
 * names of the draft are right: the small-data word is 0x1A9670
 * (`lw $v1,-0x5980($gp)`, $gp being 0x1AEFF0) and the short is 0x152CE4
 * (`lh $a1,0x2CE4($v0)` with $v0 = 0x15000000).
 *
 * What is left: 176 bytes against 168.  This cc1 peels the loop's first
 * iteration - it tests entry[1] through the base+0x4B54 displacement and
 * materialises the walk pointer afterwards - where retail emits
 * `addiu $a0,$a0,0x4B40` straight after the entry[0] test, so retail's loop
 * starts on entry[0] and steps the pointer before each load.  The loop body
 * itself is the same in both (test, pointer step, load, compare).
 *
 * Measured shapes that keep the peeling: the pointer stepped with a char *
 * cast, with `p += 5`, the pointer held in a char *, the walk pointer and the
 * call argument spelled with the symbol, and a do-while loop (which changes
 * the loop's shape instead).
 *
 * Reads a $gp word, so it belongs to the L00_002FC7A0 -g8 unit.
 */

typedef struct { char gap[0x4B40]; int entries[0x30 * 5]; } TocBase_L00_00305600;

extern TocBase_L00_00305600 D_0014FB540;

extern int D_L00_001A9670 __attribute__((sda));

extern short D_00152CE4;

extern void func_002FC3C0(int);

extern void func_002946C0(int, int, int);

int func_L00_00305600(int object, int argument)
{
    int i = 0;
    int *p;

    if (D_0014FB540.entries[0] != object) {
        p = D_0014FB540.entries;
        for (i = 1; i < 0x30; i++) {
            p = (int *)((char *)p + 0x14);
            if (*p == object)
                break;
        }
    }
    if (i == 0x30)
        return 1;
    if (D_L00_001A9670 != -1)
        return 0;
    func_002FC3C0(1);
    func_002946C0(object, D_00152CE4, argument);
    return 1;
}
