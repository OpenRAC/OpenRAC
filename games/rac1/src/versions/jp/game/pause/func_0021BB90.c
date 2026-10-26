s32 update_menu_entry_actions(struct MenuScreen_1BB90 *menu) {
    s32 focused;
    s32 entry_index;
    s32 entry_count;
    s32 previous_selection;
    s32 flags;
    s32 buttons;
    s16 fade_timer;
    s16 message_index;

    focused = menu_system_r3.current->focus == menu;
    for (entry_index = 0; menu->data.list.items[entry_index].text != 0; entry_index++) {
        if (focused && menu->data.list.selected == entry_index) {
            menu->data.list.items[entry_index].fade_timer =
                menu->data.list.items[entry_index].fade_timer + 1;
            if (menu->data.list.items[entry_index].fade_timer > 0x7D00) {
                menu->data.list.items[entry_index].fade_timer = 0x7D00;
            }
        } else {
            fade_timer = menu->data.list.items[entry_index].fade_timer;
            if (scale_game_frame_count_r(D_001602B4_1BB90) < fade_timer) {
                menu->data.list.items[entry_index].fade_timer =
                    scale_game_frame_count_r(D_001602B4_1BB90);
            }
            menu->data.list.items[entry_index].fade_timer =
                menu->data.list.items[entry_index].fade_timer > 0
                    ? menu->data.list.items[entry_index].fade_timer - 1
                    : 0;
        }
    }
    if (!focused) {
        return 0;
    }
    if (g_controller_state_r.pressed_unmasked & 0xD00) {
        if (menu->data.list.flags & 0x20) {
            selected_level_index_r[0] = g_current_level_index_r;
        }
        return -1;
    }
    if (g_controller_state_r.pressed_unmasked & 0x10) {
        if (menu->data.list.flags & 0x20) {
            selected_level_index_r[0] = g_current_level_index_r;
        }
        if (menu_system_r3.current->back != 0) {
            menu_system_r3.next = menu_system_r3.current->back;
            return 0;
        } else if (menu_system_r3.close_locked == 0) {
            return -1;
        }
    }
    if ((g_controller_state_r.pressed_unmasked & 0x60) && ((mode_freeze_flags_r ^ 1) & 1)) {
        /* Index through menu-> directly: locals for items/selected change
           which register keeps the copy of selected that actions 6 and 9 reuse. */
        switch (menu->data.list.items[menu->data.list.selected].action) {
        case 0:
            break;
        /* No case 1 in PAL: retail's table sends action 1 to the default. */
        case 3:
            menu_system_r3.next =
                (struct MenuPage_1BB90 *)menu->data.list.items[menu->data.list.selected].param.value;
            break;
        case 4:
            Moby_PlaySound_r2(0, 0x11, menu->moby);
            menu_system_r3.unk150 = 0;
            mode_freeze_flags_r = (mode_freeze_flags_r | 2) & ~4;
            mode_freeze_init_r2(3, menu->data.list.items[menu->data.list.selected].param.value, 0);
            break;
        case 5:
            Moby_PlaySound_r2(0, 0x11, menu->moby);
            mode_freeze_flags_r = (mode_freeze_flags_r | 4) & ~2;
            mode_freeze_init_r2(3, menu->data.list.items[menu->data.list.selected].param.value, 0);
            menu_system_r3.unk150 = 1;
            break;
        case 6:
            /* This action passes the low halfword; the other indexed actions pass the full word. */
            message_index = menu->data.list.items[menu->data.list.selected].param.half.hi;
            if (message_index != 0) {
                menu_system_r3.unkEC = menu_action_messages_r[message_index];
            }
            menu_system_r3.close_request = 5;
            menu_system_r3.unkF0 = menu_system_r3.current;
            menu_system_r3.unkF4 = 0;
            menu_system_r3.unkE4 =
                menu->data.list.items[menu->data.list.selected].param.half.lo;
            Moby_PlaySound_r2(0, 0x11, menu->moby);
            return 0;
        case 7:
            menu_system_r3.unkF4 = 2;
            menu_system_r3.unkF0 = menu_system_r3.current;
            menu_system_r3.close_request = 3;
            menu_system_r3.unkE4 = menu->data.list.items[menu->data.list.selected].param.value;
            Moby_PlaySound_r2(0, 0x11, menu->moby);
            return 0;
        case 8:
            menu_system_r3.unkF4 = 2;
            menu_system_r3.unkF0 = menu_system_r3.current;
            menu_system_r3.close_request = 4;
            menu_system_r3.unkE4 = menu->data.list.items[menu->data.list.selected].param.value;
            Moby_PlaySound_r2(0, 0x11, menu->moby);
            return 0;
        case 10:
            menu_system_r3.unkF4 = 2;
            menu_system_r3.unkF0 = menu_system_r3.current;
            menu_system_r3.close_request = 6;
            menu_system_r3.unkE4 = menu->data.list.items[menu->data.list.selected].param.value;
            Moby_PlaySound_r2(0, 0x11, menu->moby);
            return 0;
        case 11:
            menu_system_r3.unkF0 = menu_system_r3.current;
            menu_system_r3.unkF4 = 2;
            menu_system_r3.close_request = 7;
            Moby_PlaySound_r2(0, 0x11, menu->moby);
            return 0;
        case 9:
            g_game_language_r = menu->data.list.items[menu->data.list.selected].param.value;
            return 0;
        case 2:
            Moby_PlaySound_r2(2, 0x11, menu->moby);
            break;
        }
    }
    /* Count first, then read selected/flags: in the other order (or as a
       do-while) the allocator shifts every register of the tail. */
    for (entry_count = 0; menu->data.list.items[entry_count].text != 0; entry_count++) {
    }
    previous_selection = menu->data.list.selected;
    flags = menu->data.list.flags;
    if (flags & 1) {
        buttons = g_controller_state_r.raw_pressed;
    } else {
        buttons = g_controller_state_r.pressed_unmasked;
    }
    if ((buttons & 0x1000) || ((flags & 0x100) && (buttons & 4))) {
        if (menu->data.list.selected != 0) {
            menu->data.list.selected--;
        } else if (flags & 0x1000) {
            menu->data.list.selected = entry_count - 1;
        } else {
            menu_system_r3.current->pending_focus = menu->data.list.up;
        }
    }
    if ((buttons & 0x4000) || ((menu->data.list.flags & 0x100) && (buttons & 8))) {
        if (menu->data.list.items[menu->data.list.selected + 1].text != 0 &&
            menu->data.list.items[menu->data.list.selected + 1].action != 0) {
            menu->data.list.selected++;
        } else if (menu->data.list.flags & 0x1000) {
            menu->data.list.selected = 0;
        } else {
            menu_system_r3.current->pending_focus = menu->data.list.down;
        }
    }
    if (menu->data.list.selected != previous_selection || menu_system_r3.current->pending_focus != 0) {
        Moby_PlaySound_r2(1, 0x11, menu->moby);
        if (menu->data.list.flags & 0x20) {
            selected_level_index_r[0] = D_001602E0_1BB90[menu->data.list.selected];
        }
    }
    return 0;
}
