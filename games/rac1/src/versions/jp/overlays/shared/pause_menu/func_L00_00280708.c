extern struct MenuSystem_280708 D_J_001B9E70;
int Menu_GoBackWithFlag(void) {
    int state = mode_freeze_state;
    unsigned int pressed;

    if (state != 0x10 && state != 1) {
        D_J_001B9E70.next = D_J_001B9E70.current->back;
        return 0;
    }
    pressed = (unsigned int)g_controller_state_r2.pressed;
    if (pressed & 0x60) {
        D_J_001B9E70.unkD4 = 0;
        D_J_001B9E70.next = D_J_001B9E70.current->back;
        D_J_001B9E70.current->unk84 = 1;
    } else if (pressed & 0x10) {
        D_J_001B9E70.next = D_J_001B9E70.current->back;
        D_J_001B9E70.current->unk84 = 0;
    }
    return 0;
}
