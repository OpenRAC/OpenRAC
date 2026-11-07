void handle_pause_input(void) {
    int lvl, area, noctl, special;
    unsigned char *r;
    if (gDebugFreeCamLvl.wEC) {
        if (--gDebugFreeCamLvl.wEC == 0)
            gDebugFreeCamLvl.w8 = 0x10;
    }
    if ((gDebugFreeCamLvl.w8 & 0x10) && (((PadS *)(&g_controller_state))->w1A4 & 0x200)) {
        ((PadS *)(&g_controller_state))->w1A4 = ~gDebugFreeCamLvl.wA0 & ((PadS *)(&g_controller_state))->w1A0;
        ((PadS *)(&g_controller_state))->w1A8 = ~((PadS *)(&g_controller_state))->w1A0 & gDebugFreeCamLvl.wA0;
        gDebugFreeCamLvl.wA0 = ((PadS *)(&g_controller_state))->w1A0 & ~0x200;
    }
    lvl = cinematic_mode;
    if (lvl == 0 && (mode_freeze_flags & 0x1) && cinematic_mode_frames >= 8) {
        ((void (*)(int, int))mode_freeze_init)(3, 0);
        return;
    }
    noctl = ((PadS *)(&g_controller_state))->w19C == 0;
    if (level_menu_system_field_f0 && lvl == 0) {
        PauseAllSounds(0);
        return;
    }
    area = g_current_level_index;
    if (area == 15 && cinematic_mode_frames >= 8 && g_hero.vehicle_mode == 2 &&
        ((((PadS *)(&g_controller_state))->w1A4 & 0x800) || noctl) && g_hero.state.current != 0x72) {
        if (g_current_follow_cam->h86 == 0x13)
            return;
        ((void (*)(int, int))mode_freeze_init)(4, 0);
        return;
    }
    special = 0;
    if (g_hero.state.control_mode == 9 && g_hero.state.current == 0x32 && ((ObjS *)g_hero.ship_moby)) {
        int k = ((ObjS *)g_hero.ship_moby)->hA6;
        if (k == 0x45 || k == 0x563 || k == 0x4DA)
            special = 1;
    }
    if (special && cinematic_mode_frames >= 8 && ((((PadS *)(&g_controller_state))->w1A4 & 0x800) || noctl) &&
        g_hero.state.current != 0x72 && g_current_follow_cam->h86 != 0x13) {
        ((void (*)(int, int))mode_freeze_init)(1, 0);
        return;
    }
    if (lvl == 0 && (area == 8 || area == 0xC) && ((((PadS *)(&g_controller_state))->w1A4 & 0x800) || noctl) &&
        g_hero.state.current == 0x32) {
        ((void (*)(int, int))mode_freeze_init)(1, 0);
        return;
    }
    if (((((PadS *)(&g_controller_state))->w1A4 & 0x800) || noctl) && cinematic_mode_frames >= 8 &&
        g_hero.state.current != 0x72 && g_hero.state.control_mode == 0x16) {
        ((void (*)(int, int))mode_freeze_init)(0, 0);
        return;
    }
    if (((((PadS *)(&g_controller_state))->w1A4 & 0x800) || noctl) && cinematic_mode_frames >= 8 &&
        g_hero.state.current != 0x72 && g_hero.items[0].switch_lock != 2 && g_hero.state.current != 0x32 &&
        g_hero.state.current != 0x1D && !g_hero.fall_fade_pending && g_hero.health.hp && lvl == 0) {
        PauseAllSounds(0);
        return;
    }
    if (!(gDebugFreeCamLvl.w8 & 0x10) && cinematic_mode_frames >= 8 &&
        (((PadS *)(&g_controller_state))->w1A4 & 0x500) && g_hero.state.control_mode != 0x16 &&
        g_hero.state.current != 0x72 && g_hero.state.current != 0x32 && g_hero.state.current != 0x1D &&
        !g_hero.fall_fade_pending && g_hero.health.hp && lvl == 0 && !special && !gDebugFreeCamLvl.wEC) {
        PauseAllSounds(10);
        return;
    }
    if (((AreaS *)memory_card_state)->wFC && !(r = (unsigned char *)gSpecialItems)[0x10] &&
        cinematic_mode_frames >= 8 && !D_0015EF20 && !g_hero.fall_fade_pending && g_hero.health.hp &&
        scale_ticks(0x1E) < level_frame_counter && g_current_level_index) {
        ((void (*)(int, int))mode_freeze_init)(5, 0);
        r[0x10] = 1;
        return;
    }
    if (g_hero.state.current != 0x1D && g_hero.state.current != 0x32)
        update_timed_hud_visibility();
    if ((((PadS *)(&g_controller_state))->w1A4 & 0x200) && g_hero.state.current != 0x1D &&
        g_hero.state.current != 0x32) {
        int x;
        x = show_health_hud();
        hud_keep_visible(x, scale_ticks(0xB4));
        x = queue_animation_update(2, 0x754E, (int)initialize_bolt_counter_hud,
                                   (int)update_bolt_counter_hud_animation_and_visibility,
                                   (int)draw_bolt_counter_hud_panel, (int)&gBolts, 9999999);
        hud_keep_visible(x, scale_ticks(0xB4));
    }
    if (map_menu_state_field_24) {
        HitS *h = (HitS *)usage_stats;
        if (h->h48 < 0xFFFF)
            h->h48++;
        if (scale_ticks(gameplay_event_frame_count) / 600 > h->h4A)
            h->h4A = scale_ticks(gameplay_event_frame_count) / 600;
        h->w4C = h->w4C | (1 << g_current_level_index) | 0x80000000;
    }
    if (ship_level_exit_requested) {
        ship_level_exit_requested = 0;
        if (g_hero.health.hp) {
            prepare_level_transition_with_help_cleanup();
            return;
        }
    }
    if ((gDebugFreeCamLvl.w8 & 2) || ((gDebugFreeCamLvl.w8 & 0x10) && (((PadS *)(&g_controller_state))->w1A4 & 0x200)))
        reset_frame_counters();
    if ((gDebugFreeCamLvl.w8 & 2) ||
        ((gDebugFreeCamLvl.w8 & 0x10) && (((PadS *)(&g_controller_state))->w1A4 & 0x200))) {
        count_expired_mobys();
        update_visible_resident_objects();
        enqueue_voice_request();
        update_ambience_target_cycle();
    }
    Profile_Mark_a(level_profile_mark_after_mobys, 3);
    if ((gDebugFreeCamLvl.w8 & 1) || ((gDebugFreeCamLvl.w8 & 0x10) && (((PadS *)(&g_controller_state))->w1A4 & 0x200)))
        coordinate_hero_frame_update();
    Profile_Mark_a(level_profile_mark_after_hero, 7);
    if ((gDebugFreeCamLvl.w8 & 4) ||
        ((gDebugFreeCamLvl.w8 & 0x10) && (((PadS *)(&g_controller_state))->w1A4 & 0x200))) {
        UpdateParts();
        update_glints();
    }
    Profile_Mark_a(level_profile_mark_after_parts, 5);
    if ((gDebugFreeCamLvl.w8 & 8) ||
        ((gDebugFreeCamLvl.w8 & 0x10) && (((PadS *)(&g_controller_state))->w1A4 & 0x200))) {
        Camera_UpdateFrame();
    } else if (gDebugFreeCamLvl.w14) {
        update_debug_camera();
        if (gDebugFreeCamLvl.w14 == 2)
            update_hero_from_debug_camera();
    }
    Profile_Mark_a(level_profile_mark_after_camera, 8);
    sound_update();
    Profile_Mark_a(level_profile_mark_after_sound, 6);
    UpdateAllPointLights();
    update_resident_packed_direction();
    update_hud_elements(1);
    Help_Update();
    if ((gDebugFreeCamLvl.w8 & 2) || ((gDebugFreeCamLvl.w8 & 0x10) && (((PadS *)(&g_controller_state))->w1A4 & 0x200)))
        update_game_frame_and_idle_counters();
    if (gDebugFreeCamLvl.w18 == 2)
        Profile_Mark_b();
    if (gDebugFreeCamLvl.w18 == 3)
        Profile_Mark_b();
    if (gDebugFreeCamLvl.w18 == 4)
        Profile_Mark_b();
    if (gDebugFreeCamLvl.w18 == 5)
        Profile_Mark_b();
    if (gDebugFreeCamLvl.w18 == 6)
        Profile_Mark_b();
}
