int loading_data_menu(struct MenuScreen *arg0) {
    char *g = menu_system;
    int old = arg0->data.save.slot;

    if (((struct MenuSystem *)g)->card_op_pending != 0) {
        char *d = memory_card_state;

        if (((MemoryCardState *)d)->busy >= 3) {
            return 0;
        }
        if (((MemoryCardState *)d)->state >= 0) {
            return 0;
        }
        ((struct MenuSystem *)g)->card_op_pending = 0;
        if (*(int *)(d + 0xE4) != 0) {
            mode_freeze_flags_m |= 0x100;
            ((int (*)(int, void *))mode_freeze_init)(3, *(void **)&((struct MenuSystem *)g)->current);
            return 0;
        }
        {
            int t = sound_volume;
            char *s = voice_pool;

            *(int *)(d + 0xF4) = 1;
            *(int *)(s + 0x48) = t * 8 / 10;
            *(int *)(s + 0x4C) = music_volume_r;
            *(int *)(s + 0x50) = t * 8 / 10;
            *(int *)(s + 0x54) = t * 7 / 10;
            *(int *)(s + 0x58) = t * 7 / 10;
            *(int *)(s + 0x5C) = t;
            request_space_load(g_current_level_index);
            *(short *)screen_globals_base_r = 0;
        }
    }
    {
        PadState *pad = (PadState *)g_controller_state;
        if (*(int *)&pad->pressed_unmasked & 0xD00) {
            if (((struct MenuSystem *)menu_system)->close_locked == 0) {
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
            p = *(int *)&pad->pressed_unmasked;
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
                if (*(int *)(d2 + 0x20 + memcard_selected_slot * 0x1C) >= 0) {
                    struct MenuSystem *g2;

                    Moby_PlaySound(0, 0x11, arg0->moby);
                    *(int *)(d2 + 0xC0) = 0;
                    *(int *)(d2 + 0x14) = arg0->data.save.slot;
                    if (((MemoryCardState *)d2)->state < 0) {
                        *(int *)(d2 + 0xE0) = 0;
                        ((MemoryCardState *)d2)->state = 0xD;
                    }
                    g2 = (struct MenuSystem *)menu_system;
                    g2->card_op_pending = 1;
                    g2->card_op_text = 0x4FB6;
                }
            }
        }
    }
    if (arg0->data.save.slot != old) {
        Moby_PlaySound(1, 0x11, arg0->moby);
    }
    return 0;
}
