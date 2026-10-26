s32 sound_options_menu(struct SoundMenu_1DB30 *menu) {
    s32 previous_selection;
    s32 *sound_volume_ptr;
    s32 previous_music_volume;
    s32 previous_sound_volume;
    s32 scaled_sound_volume;

    if ((((s32)g_controller_state_r2.pressed_unmasked) & 0xD00) && (menu_system_close_locked_r == 0)) {
        return 1;
    }
    if (((s32)g_controller_state_r2.pressed_unmasked) & 0x10) {
        struct MenuPage_1DB30 *back_page = menu_system_r5.current->back;

        if (back_page != 0) {
            menu_system_r5.next = back_page;
            return 0;
        } else if (menu_system_r5.close_locked == 0) {
            return -1;
        }
    }
    previous_selection = menu->selected_option;
    if (((s32)g_controller_state_r2.pressed_unmasked) & 0x1000) {
        menu->selected_option = (previous_selection + 2) % 3;
    }
    if (((s32)g_controller_state_r2.pressed_unmasked) & 0x4000) {
        menu->selected_option = (menu->selected_option + 1) % 3;
    }
    if ((menu->selected_option != previous_selection) || (menu_system_r5.current->pending_focus != 0)) {
        func_0022ED80_1DB30(1, 0x11, menu->sound_owner);
        if (menu->flags & 0x20) {
            selected_level_index_r2 = menu_level_indices_r[menu->selected_option];
        }
    }
    previous_music_volume = music_volume_r;
    previous_sound_volume = sound_volume_r;
    sound_volume_ptr = &sound_volume_r;
    if (((s32)g_controller_state_r2.held_unmasked) & 0x2000) {
        if (menu->selected_option == 0) {
            sound_volume_r = (previous_sound_volume + 3 < 0x401) ? previous_sound_volume + 3 : 0x400;
        }
        if (menu->selected_option == 1) {
            music_volume_r = (previous_music_volume + 3 < 0x401) ? previous_music_volume + 3 : 0x400;
        }
    }
    if (((s32)g_controller_state_r2.held_unmasked) & 0x8000) {
        if (menu->selected_option == 0) {
            sound_volume_r = (sound_volume_r - 3 <= 0) ? 0 : sound_volume_r - 3;
        }
        if (menu->selected_option == 1) {
            music_volume_r = (music_volume_r - 3 <= 0) ? 0 : music_volume_r - 3;
        }
    }
    if ((previous_music_volume != music_volume_r) || (previous_sound_volume != *sound_volume_ptr)) {
        voice_pool_r.group_0_volume = *sound_volume_ptr * 8 / 10;
        voice_pool_r.group_2_volume = voice_pool_r.group_1_volume = music_volume_r;
        voice_pool_r.group_3_volume = *sound_volume_ptr * 7 / 10;
        scaled_sound_volume = *sound_volume_ptr * 7 / 10;
        voice_pool_r.group_4_volume = scaled_sound_volume;
        voice_pool_r.group_5_volume = *sound_volume_ptr;
    }
    if (((s32)g_controller_state_r2.pressed_unmasked) & 0x60) {
        if (menu->selected_option == 2) {
            playback_mode_r = !playback_mode_r;
        }
        func_0012E380_1DB30(!playback_mode_r);
        func_0022ED80_1DB30(0, 0x11, menu->sound_owner);
    }
    return 0;
}
