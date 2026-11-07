void save_card_state_create_failed(void) {
    if (!(mode_freeze_flags & 0x40)) {
        mode_freeze_state = 3;
    }
}
