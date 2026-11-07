extern void mode_freeze_init(int, int);
void func_E1_002089F8(void) {
    MemoryCardState *b = (MemoryCardState *)memory_card_state;

    if (b->busy < 3 && b->state < 0) {
        if (b->err != 0) {
            mode_freeze_init(3, 0);
            mode_freeze_state = 0x15;
            mode_freeze_flags |= 0x40;
        } else {
            mode_freeze_state = 1;
        }
    }
}
