extern short D_E1_0013E05A[];
int process_global_state_flags(void) {
    int v = pad0_pressed;

    if (v & 0x20) {
        if (mode_freeze_state == 1 || mode_freeze_state == 0x10) {
            ((struct MenuSystem *)menu_system)->next = (MenuPage *)restart_confirm_page;
        } else {
            load_level_zero_preserving_progress(-1);
            request_space_load(0);
            *(short *)D_E1_0013E05A = 1;
        }
        return 0;
    }
    if (v & 0x40) {
        unsigned char save[4];
        unsigned char tag;
        int i;

        {
            unsigned char *m = gSkillPoints;
            tag = m[0x1D];
        }
        for (i = 0; i < 4; i++) {
            save[i] = gold_bolts_collected_18[i];
        }
        {
            struct MenuSystem *g = (struct MenuSystem *)menu_system;
            memcard_RestoreGame(g->unkE0);
        }
        {
            unsigned char *m2 = gSkillPoints;
            m2[0x1D] = tag;
        }
        for (i = 0; i < 4; i++) {
            gold_bolts_collected_18[i] = save[i];
        }
        challenge_mode = 1;
        memcard_Save(0, -1);
        clear_scene_state_buffers();
        FadeToBlack(scale_game_frame_count(0x10));
        g_hero[0x20B1] = 1;
        return -1;
    }
    return 0;
}
