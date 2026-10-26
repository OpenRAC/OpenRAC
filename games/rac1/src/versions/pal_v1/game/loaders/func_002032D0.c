void LoadHudBanks(void) {
    SrcHdr *base;
    int *out;
    int *p;
    int n;
    int v;
    int size24;
    HudHdr *header;
    int bank2;
    unsigned int shift54;
    int size58;
    char *bank58;
    unsigned int shift5C;
    unsigned int shift60;
    unsigned int shift64;
    char *fname;
    int *d941c0;

    base = level_root_src;
    out = hud_bank_sizes;
    p = &base->bank[1].size;
    for (n = 0; n < 5; n++) {
        *out++ = ALIGN64(*p);
        p += 2;
    }

    fname = hud_bank_alloc_file;
    d941c0 = level_resource_buffers;
    size24 = ALIGN64(base->bank[0].size);
    header = (HudHdr *) Hud_HeapAlloc_d(size24, 0, fname, 0x23A);
    FastMemCopy(header, (void *) (base->bank[0].off + (int) base), size24);

    hud_state_arena.header = header;
    hud_state_arena.unk1C = (char *) header + header->unk04;
    bank2 = d941c0[2] + 0x60000;
    hud_state_arena.unk20 = (char *) header + header->unk08;
    hud_state_arena.unk28 = (char *) header + header->unk0C;
    hud_state_arena.unk24 = (char *) header + header->unk10;

    if (header->unk54 != 0) {
        shift54 = (unsigned int) ALIGN64(base->bank[1].size) >> 4;
        LoadCompressedHudBank(0, bank2);
        hud_state_arena.header->unk94 = Stash_SendData(
            base->bank[1].off + (int) base, shift54, shift54, (int) hud_bank1_stash_comment);
        Hud_SendResidentBank(0, (void *) bank2, 1);
    }

    size58 = hud_state_arena.header->unk58;
    if (size58 != 0) {
        bank58 = Hud_HeapAlloc_d(size58, 0, fname, 0x261);
        LoadCompressedHudBank(1, (int) bank58);
        flush_cache(0);
        LinkHudBank(1, (int) bank58);
    }

    if (hud_state_arena.header->unk5C != 0) {
        shift5C = (unsigned int) ALIGN64(base->bank[3].size) >> 4;
        hud_state_arena.header->unk9C = Stash_SendData(
            base->bank[3].off + (int) base, shift5C, shift5C, (int) hud_bank3_stash_comment);
    }

    if (hud_state_arena.header->unk60 != 0) {
        shift60 = (unsigned int) ALIGN64(base->bank[4].size) >> 4;
        hud_state_arena.header->unkA0 = Stash_SendData(
            base->bank[4].off + (int) base, shift60, shift60, (int) hud_bank4_stash_comment);
    }

    if (hud_state_arena.header->unk64 != 0) {
        shift64 = (unsigned int) ALIGN64(base->bank[5].size) >> 4;
        hud_state_arena.header->unkA4 = Stash_SendData(
            base->bank[5].off + (int) base, shift64, shift64, (int) hud_bank5_stash_comment);
    }
}
