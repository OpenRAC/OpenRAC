void load_level_zero_preserving_progress(s32 slot) {
    s32 saved;
    u32 count;
    u8 flag4;
    u8 flag5;
    s32 *items = (s32 *)0x70000150;
    u8 *bytes;
    s32 i;
    s32 j;

    copy_memory_aligned((void *)0x70000000, gold_weapon_purchased_r, 0x28);
    copy_memory_aligned((void *)0x70000030, item_available_r, 0x25);
    copy_memory_aligned((void *)0x70000060, weapon_ammo_counts_r, 0x94);
    copy_memory_aligned((void *)0x70000100, gold_bolts_collected_r, 0x50);
    copy_memory_aligned((void *)0x70000150, pause_iop_sprite_lists_r, 0x20);
    copy_memory_aligned((void *)0x70000170, vendor_stock, 0xC);
    copy_memory_aligned((void *)0x70000180, gSkillPoints_r, 0x20);
    saved = gBolts_r;
    count = D_0015EF20_27DB0;
    flag4 = gSpecialItems_r[4] != 0;
    flag5 = gSpecialItems_r[5] != 0;
    bytes = (u8 *)0x70000030;
    load_and_initialize_level_chunk();
    copy_memory_aligned(gold_weapon_purchased_r, (void *)0x70000000, 0x28);
    for (j = 0; preserved_item_available_indices[j] != -1; j++) {
        item_available_r[preserved_item_available_indices[j]] = bytes[preserved_item_available_indices[j]];
    }
    copy_memory_aligned(weapon_ammo_counts_r, (void *)0x70000060, 0x94);
    copy_memory_aligned(gold_bolts_collected_r, (void *)0x70000100, 0x50);
    for (i = 0; i < 8; i++) {
        s32 item = items[i];
        pause_iop_sprite_lists_r[i] = item_available_r[item] ? item : 0;
    }
    copy_memory_aligned(vendor_stock, (void *)0x70000170, 0xC);
    copy_memory_aligned(gSkillPoints_r, (void *)0x70000180, 0x20);
    gBolts_r = saved;
    if (flag4) {
        gSpecialItems_r[4] = 1;
        hero_max_health = 5;
    }
    if (flag5) {
        gSpecialItems_r[5] = 1;
        hero_max_health = 8;
    }
    D_0015EF20_27DB0 = count + 1;
    gHelpDeskVoice[0] = 0;
    gHelpDeskText[0] = 0;
    sce_cd_read_clock_r(save_clock_time_r);
    sce_scf_get_local_time_from_rtc_r(save_clock_time_r);
    if (slot >= 0) {
        memory_card_state_r5.card[0].x14 = slot;
        memory_card_state_r5.card[0].slots[slot].id = 0;
        memcard_MakeWholeSave_r(D_001D5F70_27DB0.unkE0);
        memory_card_state_r5.err_latch = D_001D5F70_27DB0.unkE0;
        memory_card_state_r5.cmd = 0;
        if (memory_card_state_r5.state < 0) {
            memory_card_state_r5.sub = 0;
            memory_card_state_r5.state = 0x13;
        }
    }
}
