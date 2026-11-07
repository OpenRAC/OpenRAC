void CardMenu_State11MarkSlotChoose(void) {
    MemoryCardState *b = (MemoryCardState *)memory_card_state;
    int v;
    if (b->card[0].result < -1) {
        mode_freeze_state = 3;
        return;
    }
    v = b->card[0].x14;
    if (v == -2) {
        if (b->card[0].free < 0x15E) {
            mode_freeze_state = 0x13;
        } else {
            mode_freeze_state = 0xC;
        }
        return;
    }
    if (v >= -1) {
        mode_freeze_state = 0x10;
    }
}
