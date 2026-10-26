int check_memory_card(void) {
    int type, free, format, cmd, result;

    result = sce_mc_get_info(memory_card_state_i[0], memory_card_state_i[1], &type, &free, &format);
    while (sce_mc_sync(1, &memory_card_state_i[0x30], &memory_card_state_i[0x31]) == 0) {
        sce_gs_sync_v(0);
    }
    if (memory_card_state_i[0x31] == -5 || memory_card_state_i[0x31] < -9 || type != 2) {
        return 1;
    }
    if (memory_card_state_i[0x31] != -2 && format != 0) {
        result = sce_mc_get_dir(memory_card_state_i[0], memory_card_state_i[1], memcard_save_dir_name, 0, -1, 0);
        while (sce_mc_sync(1, &cmd, &result) == 0) {
            sce_gs_sync_v(0);
        }
        if (result <= 0) {
            int t = free < 351;
            return t << 1;
        }
    }
    return 0;
}
