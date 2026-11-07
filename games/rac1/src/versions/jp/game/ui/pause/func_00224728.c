s32 saving_data_menu2(MenuScreen *w) {
    s32 old;
    s32 pad;
    s32 cursor;
    s32 cur;
    s32 f2;
    s32 one;
    MenuPage *back;

    if (menu_system_pv.current->focus != w) {
        return 0;
    }
    old = w->data.save.slot;
    if (w->data.save.step == 0 && menu_system_pv.previous == ((MenuPage *)save_menu_overwrite_confirm_page) &&
        menu_system_pv.previous->confirmed != 0) {
        w->data.save.step = 1;
    }
    if (w->data.save.step == 0 && menu_system_pv.previous == ((MenuPage *)save_menu_load_confirm_page) &&
        menu_system_pv.previous->confirmed != 0) {
        w->data.save.step = 1;
    }
    if (w->data.save.step == 1) {
        if (w->data.save.flags & 0x2000) {
            load_level_zero_preserving_progress(w->data.save.slot);
        } else {
            start_memory_card_save(w->data.save.save_data, w->data.save.slot);
        }
        menu_system_pv.card_op_pending = 1;
        menu_system_pv.card_op_text = 0x4FB6;
        memory_card_error_latch = 0;
    }
    w->data.save.step = 2;

    if (menu_system_pv.card_op_pending != 0) {
        if (memory_card_state_pv.state >= 3 || memory_card_state_pv.err >= 0 || menu_system_pv.ticks < 0xB) {
            return 0;
        }
        menu_system_pv.card_op_pending = 0;
        if (memory_card_state_pv.err_latch != 0 || memory_card_state_pv.card[0].result != 0 ||
            memory_card_error_latch != 0) {
            if (pause_restart_reset_flag != 0) {
                mode_freeze_flags_m |= 0x80;
                mode_freeze_init(8, menu_system_pv.current, memory_card_state_pv.unkFC);
                memory_card_state_pv.unkFC = 0;
                return 0;
            } else {
                mode_freeze_flags_m |= 0x80;
                memory_card_state_pv.unkFC = 0;
                mode_freeze_init(3, menu_system_pv.current, 0);
                return 0;
            }
        }
        memory_card_state_pv.unkFC = 1;
        memory_card_state_pv.card[0].slots[memory_card_state_pv.card[0].save_index].bolts = gBolts;
        memory_card_state_pv.card[0].slots[memory_card_state_pv.card[0].save_index].id = g_current_level_index_mm;
        memory_card_state_pv.card[0].slots[memory_card_state_pv.card[0].save_index].time = game_clock_ticks;
        *(SaveWord2 *)&memory_card_state_pv.card[0].slots[memory_card_state_pv.card[0].save_index].name =
            save_clock_time_s;
        memory_card_state_pv.card[0].slots[memory_card_state_pv.card[0].save_index].gold_bolts = D_0015EF20;
        request_space_load(0);
        screen_globals_base[0] = 1;
        return 0;
    }

    if ((controller_state_pv.pressed_unmasked & 0xD00) && menu_system_pv.close_locked == 0) {
        return 1;
    }
    if (controller_state_pv.pressed_unmasked & 0x10) {
        back = menu_system_pv.current->back;
        if (back != 0) {
            menu_system_pv.next = back;
            return 0;
        } else if (menu_system_pv.close_locked == 0) {
            return -1;
        }
    }
    if (mode_freeze_state != 0x10 && mode_freeze_state != 1) {
        menu_system_pv.next = menu_system_pv.current->back;
        return 0;
    }
    if (memory_card_state_pv.state >= 3 || memory_card_state_pv.err >= 0 || menu_system_pv.ticks < 0xB ||
        memory_card_state_pv.card[0].type != 2) {
        return 0;
    }

    if (w->data.save.flags & 1) {
        pad = controller_state_pv.raw_pressed;
    } else {
        pad = controller_state_pv.pressed;
    }
    w->data.save.slot = memcard_selected_slot;
    if ((pad & 0x1000) && memcard_selected_slot != 0) {
        w->data.save.slot = memcard_selected_slot - 1;
    }
    if (pad & 0x4000) {
        cur = w->data.save.slot;
        if (cur < 4) {
            w->data.save.slot = cur + 1;
        }
    }
    cursor = w->data.save.slot;
    memcard_selected_slot = cursor;
    if ((pad & 0x20) && memory_card_state_pv.card[0].type == 2) {
        if (memory_card_state_pv.card[0].slots[cursor].id != -1) {
            menu_system_pv.next = (w->data.save.flags & 0x2000) ? ((MenuPage *)save_menu_load_confirm_page)
                                                                : ((MenuPage *)save_menu_overwrite_confirm_page);
            menu_system_pv.unkD4 = (w->data.save.flags & 0x2000) ? 2 : 1;
            save_confirm_slot = w->data.save.slot;
        } else {
            w->data.save.step = 1;
        }
    } else if (pad & 0x40) {
        if (w->data.save.flags & 0x2000) {
            if (memory_card_state_pv.unkFC != 0) {
                mode_freeze_init(7, 0, 0);
                goto moved;
            }
            memory_card_state_pv.unkFC = 0;
            load_level_zero_preserving_progress(-1);
        } else {
            goto cont;
        }
    done:
        request_space_load(0);
        screen_globals_base[0] = 1;
        return 0;
    cont:
        memory_card_state_pv.unkFC = 0;
        f2 = mode_freeze_flags_m & ~2;
        mode_freeze_flags_m = f2 & ~4;
        load_and_initialize_level_chunk();
        goto done;
    }
moved:
    if (w->data.save.slot != old) {
        Moby_PlaySound(1, 0x11, w->moby);
    }
out:
    return 0;
}
