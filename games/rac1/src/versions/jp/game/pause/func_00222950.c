int update_level_selection_map_screen(void) {
    if (pad0_pressed_unmasked & 0x60) {
        pause_current_action_list = pause_default_action_list;
    }
    return 0;
}
