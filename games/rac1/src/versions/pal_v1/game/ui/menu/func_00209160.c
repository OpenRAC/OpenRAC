void save_card_state_init(void) {
    mode_freeze_state = 3;
    memory_card_state_i[0x3D] = 0;
    memory_card_state_i[0x07] = memory_card_state_i[0x2F];
}
