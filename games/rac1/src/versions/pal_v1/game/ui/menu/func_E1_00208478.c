void func_E1_00208478(void) {
    MemoryCardState *b = (MemoryCardState *)memory_card_state;

    b->state = -1;
    b->sub = -1;
    mode_freeze_state = 4;
}
