void CardMenu_State16NextState(void) {
    int old;
    int flags;
    MemoryCardState *b;
    old = mode_freeze_flags;
    mode_freeze_flags = old & ~4;
    flags = mode_freeze_flags & ~2;
    mode_freeze_flags = flags;
    if (old & 0x80) {
        mode_freeze_state = 0x15;
        mode_freeze_flags = (flags ^ 0x80) | 0x40;
        return;
    }
    if (old & 0x100) {
        mode_freeze_state = 0x14;
        mode_freeze_flags = (flags ^ 0x100) | 0x40;
        return;
    }
    b = (MemoryCardState *)memory_card_state;
    if (b->card[0].result != 0) {
        mode_freeze_state = 3;
        return;
    }
    if (*(int *)((char *)b + 0xF4) != 0) {
        mode_freeze_state = 1;
    }
}
