/* Hud_SendResidentBank(bank, addr, arg2): if the bank has no address yet,
   links bank 0 at addr (func_001FF7F0). Then, for the table2 entries from
   the previous bank's limit up to this bank's limit, sends each one to
   func_00201348 with the running position pos >> 8, tag 0x1B, the entry's
   bytes +6 and +7, and arg2. Each sent entry gets its position stored as a
   halfword at +4, and pos advances by 4 << (b6 + b7). */
extern int D_0015EF88_hud __asm__("D_0015EF88");
extern void func_00201348_i(int, int, int, int, int, int) __asm__("func_00201348");

void Hud_SendResidentBank(int bank, char *addr, int arg2) {
    HudBankHeader *h;
    int pos;
    int start;
    int end;
    int i;

    h = D_0019A4E8_arena.header;
    if (h->banks[bank] == 0) {
        func_001FF7F0(0, (int)addr);
    }
    pos = D_0015EF88_hud;
    start = (bank != 0) ? h->limits2[bank - 1] : 0;
    end = h->limits2[bank];
    if (start < end) {
        i = start;
        do {
            HudEntry *e = &D_0019A4E8_arena.table2[i];
            unsigned char b6 = *((unsigned char *)e + 6);
            unsigned char b7 = *((unsigned char *)e + 7);
            int s = pos >> 8;
            unsigned int step = 1u << ((b6 + b7) & 31);
            i++;
            func_00201348_i(e->offset, s, 0x1B, b6, b7, arg2);
            pos = (int)((unsigned int)pos + (step << 2));
            *(short *)((char *)e + 4) = (short)s;
        } while (i < end);
    }
}
