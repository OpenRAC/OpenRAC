extern int D_E1_001D5C94;
int handle_cyclic_menu_selection_input(struct MenuScreen *arg0) {
    PadState *pad = (PadState *)g_controller_state;
    int w, old;

    if (*(int *)&pad->pressed_unmasked & 0xD00) {
        if (D_E1_001D5C94 == 0) {
            return 1;
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
        w = pad3->pressed;
    }
    old = arg0->data.list.selected;
    if (w & 0x40) {
        arg0->data.list.selected = (old + 1) % 30;
    } else if (w & 0x20) {
        arg0->data.list.selected = (old + 29) % 30;
    }
    if (arg0->data.list.selected != old) {
        Moby_PlaySound(1, 0x11, arg0->moby);
    }
    return 0;
}
