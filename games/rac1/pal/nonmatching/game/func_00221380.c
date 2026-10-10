/*
 * The memory card page's picture and status draw (PAL; the US twin is the NTSC decomp's
 * draw_checking_memory_card_data_menu, which this follows, checked against the PAL assembly).
 * Before the page has its picture (state < 2): with the card busy, or in the page's first 11
 * ticks, the card text (the pending operation's, else 0x4FB9) centred in the page with its
 * shadow; returns 2 (1 when the page shows no status). After: the picture of the entry being
 * loaded, full screen, unless that entry's item is not owned (returns 1); returns 0x10.
 *
 * Offsets: the page's +0x20 width, +0x24 height, +0x34 flags (0x100 status, 4 item entry),
 * +0x38/+0x3C the picture's size, +0x44 state, +0x48/+0x4C the two picture buffers,
 * +0x50/+0x54 the loaded entries; menu system D_001D5F70 (+0x04 current page, +0x128 card op
 * pending, +0x12C its text, +0x154 ticks on the page); card state D_0013D390 (+0x08 card
 * type, +0xDC state, +0xE4 pending state); screen size at D_00151880 + 0x160.
 */
extern char D_001D5F70_21380[] __asm__("D_001D5F70");
extern char D_0013D390_21380[] __asm__("D_0013D390");
extern char D_00151880_21380[] __asm__("D_00151880");
extern unsigned char D_0013D5C8_21380[] __asm__("D_0013D5C8");
extern unsigned char D_0013D490_21380[] __asm__("D_0013D490");
extern void func_001F4630_21380(int) __asm__("func_001F4630");
extern void func_001F4748_21380(void) __asm__("func_001F4748");
extern char *func_001FE540_21380(int) __asm__("func_001FE540");
extern void *func_001153FC_21380(void *, int, unsigned int) __asm__("func_001153FC");
extern void func_001F75D0_21380(short *, u64, char *, int) __asm__("func_001F75D0");
extern u64 func_00205520_21380(int) __asm__("func_00205520");
extern void func_001F5800_21380(int, int, int, int, int, int, int, int, u64, u64) __asm__("func_001F5800");

#define MS(o) (*(int *)(D_001D5F70_21380 + (o)))
#define MC(o) (*(int *)(D_0013D390_21380 + (o)))
#define P(o) (*(int *)(page + (o)))

int func_00221380(char *page) {
    short fields[12];
    short window[12];
    char *text;
    int state = P(0x44);
    int i;

    if (state < 2) {
        if (!(P(0x34) & 0x100)) {
            return 1;
        }
        if (MC(0x08) != 2) {
            return 2;
        }
        if (MC(0xDC) < 3 && MC(0xE4) < 0 && MS(0x154) >= 0xB) {
            return 2;
        }
        func_001F4630_21380(0);
        text = func_001FE540_21380(MS(0x128) ? MS(0x12C) : 0x4FB9);
        func_001153FC_21380(fields, 0, sizeof(fields));
        fields[1] = (short)(P(0x24) + 1);
        fields[0] = 1;
        fields[2] = 1;
        fields[3] = (short)(P(0x20) + 1);
        fields[4] = (short)(P(0x20) >> 1);
        fields[5] = 5;
        fields[8] = 16;
        fields[9] = 5;
        for (i = 0; i < 12; i++) {
            window[i] = fields[i];
        }
        func_001F75D0_21380(window, 0x80000000ULL, text, -1);
        window[5] = (short)((P(0x24) - window[7]) >> 1);
        window[9] ^= 4;
        func_001F75D0_21380(window, 0x80000000ULL, text, -1);
        for (i = 0; i < 6; i++) {
            window[i]--;
        }
        func_001F75D0_21380(window, 0x80FFA888ULL, text, -1);
        func_001F4748_21380();
        return 2;
    }
    if (P(0x34) & 4) {
        int loaded = P(0x50 + (state < 4 ? 0 : 4));
        char *focus = *(char **)(MS(0x04) + 0x40);
        char *cell = *(char **)(focus + 0x48) + loaded * 10;
        short kind = *(short *)(cell + 4);
        short id = *(short *)(cell + 6);
        if (kind == 0 && D_0013D5C8_21380[id] == 0) {
            return 1;
        }
        if (kind == 1 && D_0013D490_21380[id] == 0) {
            return 1;
        }
    }
    func_001F4630_21380(0);
    func_001F5800_21380(0, 0, *(short *)(D_00151880_21380 + 0x160),
                        *(short *)(D_00151880_21380 + 0x162), 0, 0, P(0x38), P(0x3C),
                        0x80808080ULL, func_00205520_21380(state < 4 ? P(0x48) : P(0x4C)));
    func_001F4748_21380();
    return 0x10;
}
