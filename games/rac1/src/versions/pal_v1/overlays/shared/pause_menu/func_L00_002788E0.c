int Menu_CheckPageInput(void *screen) {
    if (menu_system_p.current->focus != screen) {
        return 0;
    }
    if ((controller_state_p.pressed_unmasked & 0xD00) && menu_system_p.close_locked == 0) {
        return 1;
    }
    if (controller_state_p.pressed_unmasked & 0x10) {
        struct MenuPageP *back = menu_system_p.current->back;
        if (back != 0) {
            menu_system_p.next = back;
        } else if (menu_system_p.close_locked == 0) {
            return -1;
        }
    }
    return 0;
}
