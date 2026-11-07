void save_card_state_warning(void) {
    if ((mode_freeze_flags ^ 1) & 1) {
        mode_freeze_state = 3;
    }
}
