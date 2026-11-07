int update_menu_option_list(struct MenuScreen *arg0) {
    struct MenuSystem *g = (struct MenuSystem *)menu_system;
    int sel = g->current->focus == arg0;

    if (arg0->data.options.fade_timer != 0) {
        int t = arg0->data.options.fade_timer - 1;
        arg0->data.options.fade_timer = t;
        gFade = (float)(t < 5 ? t : 4) * 0.25f;
        return 0;
    }
    if (!sel) {
        return 0;
    }
    {
        PadState *pad = (PadState *)g_controller_state;
        if (*(int *)&pad->pressed_unmasked & 0xD00) {
            if (g->close_locked == 0) {
                return 1;
            }
        }
    }
    {
        PadState *pad2 = (PadState *)g_controller_state;
        if (*(int *)&pad2->pressed_unmasked & 0x10) {
            struct MenuSystem *g2 = (struct MenuSystem *)menu_system;
            MenuPage *t = g2->current->back;
            if (t != 0) {
                g2->next = t;
            } else if (g2->close_locked == 0) {
                return -1;
            }
        }
    }
    {
        PadState *pad3 = (PadState *)g_controller_state;
        int old = arg0->data.options.selection;
        if ((*(int *)&pad3->pressed_unmasked & 0x1000) && old != 0) {
            arg0->data.options.selection = old - 1;
        }
        {
            PadState *pad4 = (PadState *)g_controller_state;
            if (*(int *)&pad4->pressed_unmasked & 0x4000) {
                int c = arg0->data.options.selection;
                if ((*(Row14 **)&arg0->data.options.list)[c + 1].w0 != 0) {
                    arg0->data.options.selection = c + 1;
                }
            }
        }
        {
            PadState *pad5 = (PadState *)g_controller_state;
            if (*(int *)&pad5->pressed_unmasked & 0x40) {
                Moby_PlaySound(0, 0x11, arg0->moby);
                if ((*(Row14 **)&arg0->data.options.list)[arg0->data.options.selection].flags & 1) {
                    if (*(unsigned char *)&requested_video_mode_i) {
                        struct MenuSystem *g3 = (struct MenuSystem *)menu_system;
                        ((int (*)(int, void *))mode_freeze_init)(6, *(void **)&g3->current);
                    } else {
                        FadeToBlack(4);
                        arg0->data.options.fade_timer = scale_game_frame_count(0x10);
                        *(unsigned char *)&requested_video_mode_i = !*(unsigned char *)&requested_video_mode_i;
                    }
                } else {
                    unsigned char *buf = (*(Row14 **)&arg0->data.options.list)[arg0->data.options.selection].buf;
                    if (buf != 0) {
                        buf[0] = !buf[0];
                    }
                }
            }
        }
        if (arg0->data.options.selection != old) {
            Moby_PlaySound(1, 0x11, arg0->moby);
        }
    }
    return 0;
}
