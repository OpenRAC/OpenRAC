extern int D_E1_001D5C94;
int update_menu_icon_list(MenuScreen *arg0) {
    {
        PadState *pad = (PadState *)g_controller_state;
        if (*(int *)&pad->pressed_unmasked & 0xD00) {
            if (D_E1_001D5C94 == 0) {
                return 1;
            }
        }
    }
    {
        PadState *pad2 = (PadState *)g_controller_state;
        if (*(int *)&pad2->pressed_unmasked & 0x10) {
            struct MenuSystem *g = (struct MenuSystem *)menu_system;
            MenuPage *t = g->current->back;
            if (t != 0) {
                g->next = t;
            } else if (g->close_locked == 0) {
                return -1;
            }
        }
    }
    {
        PadState *pad3 = (PadState *)g_controller_state;
        int v = *(int *)&pad3->pressed_unmasked;
        int old = arg0->data.icons.cursor;
        if ((v & 0x1000) && old != 0) {
            arg0->data.icons.cursor = old - 1;
        }
        if (v & 0x4000) {
            int c = arg0->data.icons.cursor + 1;
            if (c < arg0->data.icons.count) {
                arg0->data.icons.cursor = c;
            }
        }
        if (arg0->data.icons.cursor != old) {
            Moby_PlaySound(1, 0x11, arg0->moby);
        }
    }
    {
        int t = arg0->height * 16;
        if (t / 0x260 >= arg0->data.icons.count) {
            arg0->data.icons.first_y = 0x60;
        } else {
            int q = (t - 0x28) / 0x260;
            int n;
            if (arg0->data.stream.read_offset >= arg0->data.icons.cursor) {
                arg0->data.stream.read_offset = arg0->data.icons.cursor - 1;
                if (arg0->data.stream.read_offset < 0) {
                    arg0->data.stream.read_offset = 0;
                }
            }
            n = q - 2;
            if (arg0->data.stream.read_offset < arg0->data.icons.cursor - n) {
                arg0->data.stream.read_offset = arg0->data.icons.cursor - n;
            }
            arg0->data.icons.first_y = 0x190 - arg0->data.stream.read_offset * 0x260;
        }
    }
    return 0;
}
