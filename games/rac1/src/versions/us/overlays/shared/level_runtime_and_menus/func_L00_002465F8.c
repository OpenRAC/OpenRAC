extern char memory_card_state[];
void run_level_main_loop(void) {
    int i;
    int v;
    int quit;
    int full;
    int one;
    int per;
    char *q;

    if (((level_bss_end_r - level_bss_start_r) & 0xF) == 0 && ((unsigned int)level_bss_start_r & 0xF) == 0) {
        FastMemZero16(level_bss_start_r, level_bss_end_r - level_bss_start_r);
    } else {
        STUB_printf_r(level_bss_unaligned_msg_r);
        for (i = 0; i < level_bss_end_r - level_bss_start_r; i++) {
            level_bss_start_r[i] = 0;
        }
    }
    init_stash();
    cinematic_mode_frames_r = 0;
    cinematic_mode_r = 6;
    one = 1;
    init_level_runtime_state();
    ((Music *)cd_read_state_r)->track = 0;
    load_level_archive_resources(1, 0);
    ((Cam *)g_debug_menu_r)->f8 = 15;
    ((Cam *)g_debug_menu_r)->f30 = pal_mode_r;
    ((Cam *)g_debug_menu_r)->f38 = -1;
    ((Cam *)g_debug_menu_r)->fC = one;
    ((Cam *)g_debug_menu_r)->fF4 = one;
    reset_draw_config();
    initialize_level_map_archive_r();
    init_hud();
    load_misc_animations_into_stash();
    prepare_level_entry_transition();
    rendered_frame_count_r = 0;
    sce_gs_sync_v(0);
    PutDispBuffer();
    music_StartTrack(((Music *)cd_read_state_r)->track, 1, 0x400);
    if ((g_current_level_index_r == one && level_available_base_r[0] == 0) ||
        (g_current_level_index_r == LEVEL_VELDIN1 && D_0013D498_2465F8[0] != 0)) {
        music_Unpause();
    } else {
        music_Pause(0);
    }
    if (level_visit_state_r[g_current_level_index_r] == 0) {
        level_visit_state_r[g_current_level_index_r] = 1;
    }
    if (item_usage_r[g_current_level_index_r].count < 0xFFFF) {
        item_usage_r[g_current_level_index_r].count++;
    }
    per = 600;
    if (scale_ticks(gameplay_event_frame_count_r) / per > item_usage_r[g_current_level_index_r].minutes) {
        item_usage_r[g_current_level_index_r].minutes = scale_ticks(gameplay_event_frame_count_r) / 600;
    }
    item_usage_r[g_current_level_index_r].flags =
        item_usage_r[g_current_level_index_r].flags | (1 << g_current_level_index_r) | 0x80000000;
    snd_reenable_command_flush();

    while (level_loop_exit_requested_r == 0) {
        quit = 0;
        full = 0;
        frm_clock_time_r += (unsigned int)*(volatile int *)0x10000800;
        *(volatile int *)0x10000800 = 0;
        frame_roll_counter();
        skip_frame_r = 0;
        VU1_sendChain();
        VU1_swapChain();
        PutDrawBufferSmall();
        AA_BlurPass();
        PutDrawBufferLarge();
        snd_FlushSoundCommands();
        v = cinematic_mode_r;
        switch (v) {
            case -1:
                UpdatePadL00();
                update_debug_menu();
                draw_debug_menu();
                break;
            case 0:
                UpdatePadL00();
                handle_pause_input();
                draw_debug_profiler_if_enabled();
                break;
            case 1:
                play_queued_movie();
                break;
            case 2:
                UpdatePadL00();
                update_cutscene_playback();
                prepare_debug_profiler_render();
                break;
            case 3:
                UpdatePadL00();
                update_pause_menu();
                draw_page_contents();
                break;
            case 4:
                UpdatePadL00();
                update_mode_freeze();
                draw_dialog_background_and_text();
                break;
            case 5:
                UpdatePadL00();
                update_vendor_menu();
                draw_vendor_screen();
                break;
            case 6:
                UpdatePadL00();
                update_resident_gameplay_state();
                dispatch_game_state_update();
                break;
            case 7:
                UpdatePadL00();
                update_slideshow();
                draw_framebuffer_with_timed_text();
                break;
        }
        if (cinematic_mode_r != v) {
            cinematic_mode_frames_r = 0;
        } else {
            cinematic_mode_frames_r = cinematic_mode_frames_r + 1;
        }
        if (skip_frame_r != 0) {
            VU1_syncChain(1);
            VU1_initChain();
            continue;
        }
        memcard_update_state();
        Menu_RunStateHandler_r();
        if (*(int *)((q = memory_card_state) + 0xD4) >= 3 || *(int *)(q + 0xDC) >= 0) {
            if (((Gs *)level_menu_system_r)->f13C == 0) {
                int n = 55;
                float size;
                float ang;
                int t;

                SetupGifPaging(0);
                size = 272.0f;
                VU1_addGSregister(0x47, 0x3004B);
                t = GetIconFrame(0x755D, 0);
                draw_hud_sprite(t, 0x32, 0x12C, 0x40, 0x40, 0x80);
                ang = (float)(rendered_frame_count_r % n) * -6.2831855f / (float)n;
                draw_rotated_sprite_r(1312.0f, 5312.0f, 0x40, 0x40, GetFrameTex(GetIconFrame(0x755D, 1)), size, size,
                                      ang);
                DoGifPaging();
            }
        }
        ((Gs *)level_menu_system_r)->f13C = 0;
        flush_debug_text_queue();
        Profile_MarkL00_b(level_main_loop_profile_label_r, 0x10);
        Profile_MarkL00_c(level_main_loop_profile_label_r, 0x10);
        VU1_syncChain(1);
        {
            cpu_frame_load_r = IntToFloat(*(volatile int *)0x10000800) / (pal_mode_r != 0 ? 11520.0f : 9600.0f);
        }
        if (cinematic_mode_r == 0 && ((Cam *)g_debug_menu_r)->f18 != 0) {
            func_001E9768_v_2465F8();
        }
        if (((Hero *)g_hero_r)->fall_fade_pending == 0) {
            int mode = cinematic_mode_r;

            if (mode == 0 || mode == 2) {
                int lim = 9600;

                if (pal_mode_r != 0) {
                    lim = 11520;
                }
                if (lim < *(volatile int *)0x10000800) {
                    if (mode == 0) {
                        UpdatePadL00();
                        handle_pause_input();
                    } else if (mode == 2 && D_L00_0016C9B0_2465F8 == 0) {
                        UpdatePadL00();
                        update_cutscene_playback();
                    }
                }
                sce_gs_sync_v(0);
                rendered_frame_count_r = rendered_frame_count_r + 1;
                tick_space_transition();
                level_game_clock_paused_r = 0;
            } else {
                sce_gs_sync_v(0);
                rendered_frame_count_r = rendered_frame_count_r + 1;
                tick_space_transition();
                level_game_clock_paused_f_2465F8 = 0.0f;
            }
        }
        ResetGsRegistersPr();
        if (cinematic_mode_r == 0 && ((Hero *)g_hero_r)->fall_fade_pending != 0) {
            quit = 1;
            full = 1;
        }
        if (quit != 0) {
            VU1_initChain();
            snd_reset_state_and_flush_commands();
            snd_FlushSoundCommands();
            sound_StopAllSounds();
            music_Stop();
            load_level_archive_resources(0, full);
            cinematic_mode_r = 0;
            music_StartTrack(((Music *)cd_read_state_r)->track, 1, 0x400);
            init_hud();
        }
        per = 600;
        if (scale_ticks(gameplay_event_frame_count_r) / per > item_usage_r[g_current_level_index_r].minutes) {
            item_usage_r[g_current_level_index_r].minutes = scale_ticks(gameplay_event_frame_count_r) / 600;
        }
    }
    DoSpaceTransition();
}
