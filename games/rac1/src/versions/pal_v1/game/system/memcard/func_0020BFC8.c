extern char D_E1_001506D0[];
int memcard_Save(int slot, int flags) {
    unsigned char saved;

    sce_cd_read_clock(save_clock_time);
    sce_scf_get_local_time_from_rtc(save_clock_time);
    copy_level_object_positions();
    save_map_coverage(map_coverage_pages + (g_current_level_index_m << 11));
    {
        char *p = memory_card_state;
        int active = ((struct MemoryCardState *)p)->cmd;
        int mask;

        if (active == -1 || *(int *)(p + active * 0xB8 + 0x14) < 0) {
            return slot == 0;
        }
        mask = *(int *)(p + 0xF4) | slot;
        *(int *)(p + 0xF4) = mask;
        if (mask == 0) {
            goto done;
        }
        if (slot == 0) {
            mode_freeze_flags |= 0x200;
        }
        if (((struct MemoryCardState *)p)->busy >= 3) {
            goto done;
        }
        if (((struct MemoryCardState *)p)->state >= 0) {
            goto done;
        }
        ((struct MemoryCardState *)p)->xC8 = g_current_level_index_m;
        saved = 0;
        if (flags >= 0) {
            g_current_level_index_m = flags;
            saved = level_visit_state[flags];
            if (saved == 0) {
                level_visit_state[flags] = 1;
            }
        }
    }
    {
        char *q = memory_card_state;
        char *names = q + 0x30;

        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x24) = gBolts;
        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x20) = g_current_level_index_m;
        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x2C) = game_clock_ticks;
        memcpy(names + *(int *)(q + 0x14) * 0x1C, save_clock_time, 8);
        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x28) = D_0015EF20;
        memcard_PrepData(memcard_game_data_buffer, 0, mc_gamedata);
        memcard_PrepData(D_E1_001506D0, ((struct MemoryCardState *)q)->xC8, mc_leveldata);
        if (flags >= 0) {
            level_visit_state[g_current_level_index_m] = saved;
            g_current_level_index_m = ((struct MemoryCardState *)q)->xC8;
        }
        if (((struct MemoryCardState *)q)->state < 0) {
            ((struct MemoryCardState *)q)->state = 0xF;
            ((struct MemoryCardState *)q)->sub = ((struct MemoryCardState *)q)->cmd;
        }
    }
done: {
    struct MemoryCardState *r = (struct MemoryCardState *)memory_card_state;
    return r->state == 0xF;
}
}
