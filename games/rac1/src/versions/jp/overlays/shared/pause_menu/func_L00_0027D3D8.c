int pause_menu_entry_input(char *entry) {
    unsigned int pressed = g_controller_state_r.pressed_unmasked;
    if (pressed & 0xD00) {
        if (*(int *)(entry + 0x30) & 0x20)
            map_menu_state_target_level = g_current_level_index_m;
        return -1;
    }
    if (pressed & 0x10) {
        struct MenuPageP *back;
        if (*(int *)(entry + 0x30) & 0x20)
            map_menu_state_target_level = g_current_level_index_m;
        back = level_menu_system_r.current->back;
        if (back) {
            level_menu_system_r.next = back;
            return 0;
        }
        if (!level_menu_system_r.close_locked)
            return -1;
    }
    if (g_controller_state_r.pressed_unmasked & 0x60) {
        memory_card_state_field_fc = 0;
        memcard_last_save_slot = -1;
        level_loop_exit_requested = 1;
        skip_frame = 1;
    }
    return 0;
}
