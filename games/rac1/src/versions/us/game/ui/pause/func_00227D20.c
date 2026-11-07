void start_memory_card_save(int arg0, int arg1) {
    load_and_initialize_level_chunk();
    sce_cd_read_clock(save_clock_time);
    sce_scf_get_local_time_from_rtc(save_clock_time);
    memcard_MakeWholeSave((char *)arg0);
    {
        MemoryCardState *b = (MemoryCardState *)memory_card_state;
        b->cmd = 0;
        b->card[0].x14 = arg1;
        b->card[0].slots[arg1].id = 0;
        b->padF4 = 1;
        b->err_latch = arg0;
        if (b->state < 0) {
            b->sub = 0;
            b->state = 0x13;
        }
    }
}
