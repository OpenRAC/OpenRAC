void CardMenu_State04AdvanceOnStatus(void) {
    MemoryCardState *b = (MemoryCardState *)memory_card_state;
    int v;
    mode_freeze_flags &= ~0x20;
    v = b->card[0].result;
    if (v == 0) {
        mode_freeze_state = 9;
    } else if (v == -1) {
        b->card[0].result = 0;
        mode_freeze_state = 9;
    } else if (v == -2) {
        mode_freeze_state = 5;
    }
}
