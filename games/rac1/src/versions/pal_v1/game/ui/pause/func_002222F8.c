int update_menu_choice_list(struct MenuScreen *arg0) {
    struct MenuSystem *g = (struct MenuSystem *)menu_system;

    if (g->current->focus != arg0) {
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
        int old = arg0->data.choices.selection;
        if ((*(int *)&pad3->pressed_unmasked & 0x1000) && old != 0) {
            arg0->data.choices.selection = old - 1;
        }
        {
            PadState *pad4 = (PadState *)g_controller_state;
            if (*(int *)&pad4->pressed_unmasked & 0x4000) {
                int c = arg0->data.choices.selection;
                if ((*(Row18 **)&arg0->data.choices.list)[c + 1].w0 != 0) {
                    arg0->data.choices.selection = c + 1;
                }
            }
        }
        if (old != arg0->data.choices.selection) {
            Moby_PlaySound(1, 0x11, arg0->moby);
        }
    }
    {
        Row18 *rec = &(*(Row18 **)&arg0->data.choices.list)[arg0->data.choices.selection];
        int n = 0;
        PadState *pad5;
        int *e;

        e = rec->e;
        if (e[0] != 0) {
            do {
                n++;
            } while (e[n] != 0 && n < 4);
        }
        pad5 = (PadState *)g_controller_state;
        if (*(int *)&pad5->pressed_unmasked & 0x40) {
            unsigned char *buf = rec->buf;
            if (buf != 0) {
                buf[0] = (buf[0] + 1) % n;
                Moby_PlaySound(0, 0x11, arg0->moby);
            }
        }
    }
    return 0;
}
