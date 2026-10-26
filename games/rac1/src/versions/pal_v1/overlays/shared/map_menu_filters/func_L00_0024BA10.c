void SetMenuStateFromCardStatus(void) {
    char *b = special_items_base + 0x3B;
    if (*(int *)(b + 0xD4) == 2 && *(int *)(b + 0xDC) < 0) {
        if (*(int *)(b + 0xE4) != 0) {
            mode_freeze_state = 17;
            mode_freeze_flags |= 0x40;
        } else {
            mode_freeze_state = 14;
        }
    }
}
