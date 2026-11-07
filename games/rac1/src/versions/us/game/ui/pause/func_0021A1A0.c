void update_menu_system(void) {
    s32 i;
    s32 j;
    s32 k;
    s32 flag;
    MenuScreen *obj;
    MenuScreen **objs;
    MenuScreen *m;

    menu_system_r2.update_count = menu_system_r2.update_count + 1;
    reset_active_camera_position_and_basis_r();
    if (menu_system_r2.state == 0x14) {
        if (menu_system_r2.timer != 0) {
            menu_system_r2.timer = menu_system_r2.timer - 1;
            if (menu_system_r2.timer != 0) {
                return;
            }
        }
        if (cd_read_active_r[0] != 0) {
            return;
        }
        vu1_chain_reserved_bytes = 0x1E000;
        select_level_vu_chain();
        UpdateFog();
        space_load_in_progress_r = 1;
        menu_system_r2.help_text_buffer = 0;
        menu_system_r2.unk10C = 0;
        menu_system_r2.unk104 = 0;
        menu_system_r2.unk10 = 0;
        g_game_mode_r = 0;
        return;
    }
    if (menu_system_r2.state == 0 || menu_system_r2.state == 0x2D) {
        initialize_menu_preview_scene_r();
    }
    if (mode_freeze_flags_r & 1) {
        ((void (*)(s32, MenuPage *))mode_freeze_init_r)(3, menu_system_r2.current);
        return;
    }
    if (menu_system_r2.state == 1) {
        menu_system_r2.timer = (menu_system_r2.timer < 1) ? 0 : menu_system_r2.timer - 1;
        if (menu_system_r2.timer == 0) {
            MenuPage *o8 = menu_system_r2.next;
            menu_system_r2.next = 0;
            menu_system_r2.current = o8;
            menu_system_r2.state = o8->state;
            for (i = 0; i < 14; i++) {
                obj = menu_system_r2.current->screens[i];
                if (obj != 0 && obj->enter != 0) {
                    obj->enter(obj, 0);
                }
            }
        }
    } else if (menu_system_r2.next != 0) {
        if (menu_system_r2.current == menu_system_r2.next) {
            Moby_PlaySound_r(3, 0x11, panel_slots_r2[0]);
        } else {
            Moby_PlaySound_r(4, 0x11, panel_slots_r2[0]);
        }
        for (i = 0; i < 14; i++) {
            obj = menu_system_r2.current->screens[i];
            if (obj != 0 && obj->leave != 0) {
                obj->leave(obj, 0);
            }
        }
        flag = (menu_system_r2.next == menu_system_r2.current->back);
        if (menu_system_r2.current == menu_system_r2.next) {
            flag = flag ^ 1;
        }
        for (j = 0; j < 14; j++) {
            if (menu_system_r2.next->screens[j] != 0) {
                menu_system_r2.next->screens[j]->moby = (s32)panel_slots_r2[j];
            }
            if (flag) {
                s32 n = menu_system_r2.current->moby_anims[j];
                MobySetAnimation_r2(panel_slots_r2[j], n, panel_slots_r2[j]->unk24->unk48[n]->unk10 - 1);
                panel_slots_r2[j]->unk58 = -1.0f;
            } else {
                MobySetAnimation_r2(panel_slots_r2[j], menu_system_r2.next->moby_anims[j], 0);
                panel_slots_r2[j]->unk58 = 1.0f;
            }
        }
        menu_system_r2.state = 1;
        menu_system_r2.timer = 12;
        menu_system_r2.previous = menu_system_r2.current;
        menu_system_r2.current = 0;
    }
    if (menu_system_r2.current != 0) {
        objs = menu_system_r2.current->screens;
        for (k = 0; k < 14; k++) {
            if (objs != 0 && objs[k] != 0 && objs[k]->update != 0) {
                if (objs[k]->update(objs[k]) != 0) {
                    menu_system_r2.close_request = 1;
                }
            }
        }
        if (menu_system_r2.current != 0 && menu_system_r2.current->pending_focus != 0) {
            m = menu_system_r2.current->focus;
            if (m->leave != 0) {
                m->leave(m, 1);
            }
            menu_system_r2.current->focus = menu_system_r2.current->pending_focus;
            menu_system_r2.current->pending_focus = 0;
            m = menu_system_r2.current->focus;
            if (m->enter != 0) {
                m->enter(m, 1);
            }
        }
    }
    update_visible_resident_objects();
    if (menu_system_r2.close_request != 0) {
        close_menu_system_r();
    }
}
