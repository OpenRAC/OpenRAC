/*
 * Tier: opus-1 (Claude Opus 5.5, xhigh). Retained near miss; not linked or counted.
 * Profile: -O2 -G0 -ffunction-sections, in place in src/levels/shared/L00_002D7940.c
 * (that unit has no common.h; this draft's declarations go above the function).
 * Best check.py: 468/468 bytes, 0/27 places, two instructions differ, +0xC0 and +0x194:
 * this compiler's assembler expands cc1's `dli $a1,0xffffffff` to lui $a1,0xFFFF; dsrl32,
 * retail has addiu $a1,$zero,-1; dsrl32. Everything else agrees, registers included.
 *
 * What would unblock it: an assembler correction, not C. Retail's game code builds this
 * value with addiu -1 at all four sites (func_L00_002E3340 once, this function twice,
 * func_L03_0041CA78 once), as rac1's model of ps2eeas predicts for a low mask
 * (tools/ps2eeas_dli.py there). No game code in the boot or the levels has the lui form;
 * it occurs only in SDK objects (func_0011FC68, func_001213B8, func_00121450,
 * func_00121AB8, func_00122B00), so the SDK profile has to keep it.
 * Constant spellings and `unsigned long white = -1; white >>= 32;` all fold to the same dli.
 *
 * func_0027DE50's colour is 64-bit, as its matched definition in src/game/0027DE50-sdata.c
 * declares; func_L00_002E3340 needs that for its allocation.
 *
 * Draws the current caption of the level's caption table: the text, six shaded bars
 * shrinking by 5/6 below it, then the text again on top.
 */
typedef struct {
    int key, base, a08, a0c, width, top, mode, count, step, duration;
} DrawRec;
typedef struct {
    int buffers[7];
    short buffer, pad1e, index, pad22, active;
    char pad26[0x384 - 0x26];
    int style;
} DrawState;
extern DrawState D_L00_001BC200;
extern DrawRec D_L00_001BC588[];
extern int D_001A7340 __attribute__((sda));
extern int func_L00_002E36F8(int);
extern int func_002798A0(int, int *, int *);
extern void func_002802E0(void *, int, int, int, int, int, int, int, int);
extern void func_00280190(void *, unsigned long, int, int);
extern void func_0027DE50(int, int, int, int, unsigned long);

void func_L00_002E3520(void)
{
    int text = func_L00_002E36F8(D_L00_001BC588[D_L00_001BC200.index].key);
    int box[8];
    int i, y, bottom;

    if (text) {
        func_002802E0(box, 0, D_L00_001BC588[D_L00_001BC200.index].top, 0, D_001A7340, D_001A7340 / 2,
                      D_L00_001BC588[D_L00_001BC200.index].width, 18, 1);
        func_00280190(box, 0xFFFFFFFFUL, text, -1);
        bottom = D_L00_001BC588[D_L00_001BC200.index].top - 2;
        y = D_L00_001BC588[D_L00_001BC200.index].top - 4;
        for (i = 0; i < 6; i++) {
            func_0027DE50(bottom - y, bottom, -D_001A7340, D_001A7340 * 2, 0x18000000);
            y = y * 5 / 6;
        }
        func_002802E0(box, 0, D_L00_001BC588[D_L00_001BC200.index].top, 0, D_001A7340, D_001A7340 / 2,
                      D_L00_001BC588[D_L00_001BC200.index].width, 18, 1);
        func_00280190(box, 0xFFFFFFFFUL, text, -1);
    }
}
