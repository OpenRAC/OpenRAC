extern int D_E1_001D5C94;
int update_menu_cycle_selection(struct MenuScreen *arg0) {
    PadState *pad = (PadState *)g_controller_state;
    int w;

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
    if (w & 0x2040) {
        arg0->data.cycle.selection = (arg0->data.cycle.selection + 1) % 12;
        Moby_PlaySound(1, 0x11, arg0->moby);
    } else if (w & 0x8020) {
        arg0->data.cycle.selection = (arg0->data.cycle.selection + 11) % 12;
        Moby_PlaySound(1, 0x11, arg0->moby);
    }
    return 0;
}
