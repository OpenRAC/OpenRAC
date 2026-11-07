int saving_data_menu(struct MenuScreen *arg0) {
    struct MenuSystem *g = (struct MenuSystem *)menu_system;
    int s;
    int old;

    if (g->current->focus != arg0) {
        return 0;
    }
    if (arg0->data.save.step == 0) {
        MenuPage *m = g->previous;
        if (m == (MenuPage *)save_overwrite_confirm_page) {
            if (m->confirmed != 0) {
                arg0->data.save.step = 1;
            }
        }
    }
    s = arg0->data.save.step;
    if (s == 1) {
        struct MenuSystem *g2;

        prepare_save_game(arg0->data.save.save_data, arg0->data.save.slot);
        g2 = (struct MenuSystem *)menu_system;
        g2->card_op_pending = s;
        g2->card_op_text = 0x4FB5;
    }
    arg0->data.save.step = 2;
    old = arg0->data.save.slot;
    {
        char *g3 = menu_system;

        if (((struct MenuSystem *)g3)->card_op_pending != 0) {
            char *d = memory_card_state;
            char *e;

            if (((MemoryCardState *)d)->busy >= 3) {
                return 0;
            }
            if (((MemoryCardState *)d)->state >= 0) {
                return 0;
            }
            ((struct MenuSystem *)g3)->card_op_pending = 0;
            if (*(int *)(d + 0xE4) != 0) {
                mode_freeze_flags_m |= 0x80;
                ((int (*)(int, void *))mode_freeze_init)(3, *(void **)&((struct MenuSystem *)g3)->current);
                return 0;
            }
            e = d + 0x30;
            *(int *)(d + 0x24 + *(int *)(d + 0x14) * 0x1C) = gBolts;
            *(int *)(d + 0x20 + *(int *)(d + 0x14) * 0x1C) = g_current_level_index_mm;
            *(int *)(d + 0x2C + *(int *)(d + 0x14) * 0x1C) = game_clock_ticks;
            *(SaveWord2 *)(e + *(int *)(d + 0x14) * 0x1C) = save_clock_time_s;
            *(int *)(d + 0x28 + *(int *)(d + 0x14) * 0x1C) = D_0015EF20;
        }
    }
    {
        PadState *pad = (PadState *)g_controller_state;
        if (*(int *)&pad->pressed_unmasked & 0xD00) {
            struct MenuSystem *g2 = (struct MenuSystem *)menu_system;
            if (g2->close_locked == 0) {
                return 1;
            }
        }
    }
    {
        PadState *pad = (PadState *)g_controller_state;
        if (*(int *)&pad->pressed_unmasked & 0x10) {
            struct MenuSystem *g2 = (struct MenuSystem *)menu_system;
            MenuPage *t = g2->current->back;
            if (t != 0) {
                g2->next = t;
            } else if (g2->close_locked == 0) {
                return -1;
            }
        }
    }
    if (mode_freeze_state != 0x10 && mode_freeze_state != 1) {
        struct MenuSystem *g2 = (struct MenuSystem *)menu_system;
        g2->next = g2->current->back;
        return 0;
    }
    {
        char *d = memory_card_state;
        int p;
        int x;

        if (((MemoryCardState *)d)->busy >= 3) {
            return 0;
        }
        if (((MemoryCardState *)d)->state >= 0) {
            return 0;
        }
        if (*(int *)(d + 8) != 2) {
            return 0;
        }
        if (arg0->data.save.flags & 1) {
            PadState *pad = (PadState *)g_controller_state;
            p = pad->raw_pressed;
        } else {
            PadState *pad = (PadState *)g_controller_state;
            p = pad->pressed;
        }
        x = memcard_selected_slot;
        arg0->data.save.slot = x;
        if ((p & 0x1000) && x != 0) {
            arg0->data.save.slot = x - 1;
        }
        if ((p & 0x4000) && arg0->data.save.slot < 4) {
            arg0->data.save.slot = arg0->data.save.slot + 1;
        }
        memcard_selected_slot = arg0->data.save.slot;
        if (p & 0x40) {
            char *d2 = memory_card_state;

            if (*(int *)(d2 + 8) == 2) {
                if (*(int *)(d2 + 0x20 + memcard_selected_slot * 0x1C) != -1) {
                    struct MenuSystem *g2 = (struct MenuSystem *)menu_system;

                    g2->unkD4 = 0;
                    *(char **)&g2->next = save_overwrite_confirm_page;
                    save_confirm_slot = arg0->data.save.slot;
                } else {
                    arg0->data.save.step = 1;
                }
            }
        }
    }
    if (arg0->data.save.slot != old) {
        Moby_PlaySound(1, 0x11, arg0->moby);
    }
    return 0;
}
