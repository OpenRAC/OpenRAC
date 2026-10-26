int rebuild_configured_text_label_list(void) {
    int ids[8];
    int names[7];
    int i, n;

    *(Blk32 *)ids = cheat_menu_ids_init;
    *(Blk28 *)names = cheat_menu_text_ids_init;
    n = 0;
    for (i = 0; i < 12 && ids[i] != -1; i++) {
        if (selector_available[ids[i]] != 0) {
            cheat_toggle_menu_items[n].val = names[i];
            cheat_toggle_menu_items[n].addr = &gCheats[ids[i]];
            cheat_toggle_menu_items[n].c1 = 0x4F5B;
            cheat_toggle_menu_items[n].c2 = 0x4F5C;
            cheat_toggle_menu_items[n].zero = 0;
            n++;
        }
    }
    cheat_toggle_menu_items[n].val = 0;
    return 0;
}
