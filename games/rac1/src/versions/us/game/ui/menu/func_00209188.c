void save_card_state_good_save(void) {
    int flags = mode_freeze_flags;
    char *b;
    int nf;
    mode_freeze_flags = flags & ~4;
    b = memory_card_state;
    nf = mode_freeze_flags & ~2;
    mode_freeze_flags = nf;
    if (*(int *)(b + 0xF4) == 0) {
        mode_freeze_state = 3;
        return;
    }
    if (flags & 0x80) {
        mode_freeze_state = 0x15;
        mode_freeze_flags = (nf ^ 0x80) | 0x40;
        return;
    }
    if (flags & 0x100) {
        mode_freeze_state = 0x14;
        mode_freeze_flags = (nf ^ 0x100) | 0x40;
        return;
    }
    if (*(int *)(b + 0x1C) != 0) {
        *(int *)(b + 0xF4) = 0;
        mode_freeze_flags = nf | 1;
        mode_freeze_state = 2;
        return;
    }
    if (flags & 0x200) {
        mode_freeze_flags = nf ^ 0x200;
        mode_freeze_state = 0x16;
    }
}
