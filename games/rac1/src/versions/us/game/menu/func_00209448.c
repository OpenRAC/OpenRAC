void save_card_state_formatting(void) {
    char *s = memory_card_state;
    if (*(int *)(s + 0xD4) == 2 && *(int *)(s + 0xDC) < 0) {
        if (*(int *)(s + 0xE4) != 0) {
            mode_freeze_state = 0x11;
            mode_freeze_flags |= 0x40;
            return;
        }
        mode_freeze_state = 0xE;
    }
}
