void update_moby_list_clear_notification(unsigned char *moby) {
    int *d = *(int **)(moby + 0x78);
    unsigned v;
    int i;
    if (d[0] != -1) {
        if (count_active_moby_group_members(d[0], -1) == 0) {
            if (gSkillPoints[0x17] == 0) {
                gSkillPoints[0x17] = 1;
                PlayBankSound(1, 0, 0);
                ShowBanner(0x53D6, -1);
            }
        }
    }
    switch (moby[0x20]) {
    case 0:
        moby[0x30] = 0xFF;
        moby[0x20] = 1;
        break;
    case 1:
        if (oltanis_game_mode == 2) {
            v = oltanis_cutscene_state.state - 3;
            if (v < 3) {
                i = 0;
                if (v <= oltanis_game_mode) i = moby[0x20];
                cutscene_set_camera_target(oltanis_cutscene_state.v178[i]);
            }
        }
        break;
    }
}
