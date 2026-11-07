void prepare_save_game(int arg0, int arg1) {
    MemoryCardState *b = (MemoryCardState *)memory_card_state;
    int one;
    sce_cd_read_clock(save_clock_time);
    sce_scf_get_local_time_from_rtc(save_clock_time);
    one = 1;
    copy_level_object_positions();
    save_map_coverage(map_coverage_pages + (g_current_level_index << 11));
    memcard_MakeWholeSave((char *)arg0);
    b->padF4 = one;
    b->err_latch = arg0;
    b->card[0].x14 = arg1;
    b->cmd = 0;
    if (b->state < 0) {
        b->sub = 0;
        b->state = 0x13;
    }
}
