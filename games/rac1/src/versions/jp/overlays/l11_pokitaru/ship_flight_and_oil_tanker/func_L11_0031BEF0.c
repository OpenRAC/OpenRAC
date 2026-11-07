void cleared_group_dialogue_update(char *moby) {
    int *data = *(int **)(moby + 0x78);
    if (data[0] == -1) {
        DeleteMoby(moby);
    } else if (data[1] != -1 && count_active_moby_group_members(data[1], -1) == 0) {
        unsigned char *g = gSkillPoints;
        if (g[0x13] == 0) {
            g[0x13] = 1;
            allocate_voice_for_bank_entry(1, 0, 0);
            queue_dialogue_message(0x542D, -1);
        }
    }
}
