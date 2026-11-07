void func_E1_00208310(void) {
    int old = mode_freeze_state;

    menu_state_handlers[old]();
    menu_state_frame_count++;
    if (mode_freeze_state != old) {
        menu_state_frame_count = 0;
    }
}
