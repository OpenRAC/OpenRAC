void DoSpaceTransition(void) {
    int lvl;
    int ok;
    int done;

    lvl = 5;
    coll_output.f10 |= 0x80000000;
    g_game_mode = 6;
    level_render_state.set = 0;
    if (level_available_b.f8 != 0 || space_load_level >= 8) {
        level_render_state.set = 1;
    }
    if (level_available_b.fE != 0 || space_load_level >= 14) {
        level_render_state.set = 2;
    }
    flush_cache(0);
    snd_set_reverb_ex(2, 0, 0, 0, 0);
    snd_reset_state_and_flush_commands();
    snd_FlushSoundCommands();
    sound_StopAllSounds();
    music_Stop();
    cd_read_state_b.fB = 1;
    if (level_sound_defs != 0) {
        snd_UnloadBank(*(int *)(level_sound_defs + 0x1C));
        snd_ResolveBankXREFS();
        STUB_printf(unloaded_sound_bank_format, *(int *)(level_sound_defs + 0x1C));
    }
    sound_bank_handle_m = 0;
    snd_StreamSafeCdCallback(0);
    snd_StreamSafeCdSync(0);
    view_context_b.f238 = 16;
    view_context_b.f21C = 524288.0f;
    view_context_b.f228 = 255.0f;
    view_context_b.f230 = 0;
    view_context_b.f234 = 0;
    view_context_b.f218 = 0;
    view_context_b.f22C = 128.0f;
    SetBackgroundColor(0, 0, 0);
    if (space_load_level < 0) {
        while (memory_card_state_b.fDC >= 3 || memory_card_state_b.fE4 >= 0) {
            memcard_update_state();
            Menu_RunStateHandler();
        }
        FadeToBlack(scale_game_frame_count(6));
        g_current_level_index_m = space_load_level;
        load_level_chunk_from_disc();
        sce_cd_sync_s(0);
        cd_read_state_b.fB = 0;
        DMAC_VIF1_Disable();
        return;
    }
    if (space_load_level == 0 && level_visit_state[0] == 0) {
        FadeToBlack(scale_game_frame_count(6));
        play_level_loading_slides(lvl, 0, 1, scale_game_frame_count(240), 0);
        play_level_transition_movie(0);
        play_level_loading_slides(lvl, 2, 2, scale_game_frame_count(180), 0);
        play_level_transition_movie(1);
        g_current_level_index_m = space_load_level;
        play_level_loading_slides(lvl, 3, 4, scale_game_frame_count(240), 1);
        play_level_transition_movie(2);
    } else if (g_current_level_index_m == LEVEL_VELDIN1 && space_load_level == 1 && level_visit_state[1] == 0) {
        FadeToBlack(scale_game_frame_count(6));
        play_level_loading_slides(lvl, 5, 6, scale_game_frame_count(240), 0);
        play_level_transition_movie(3);
        play_level_transition_movie(4);
        play_level_loading_slides(lvl, 7, 7, scale_game_frame_count(180), 0);
        play_level_transition_movie(5);
        level_visit_state[g_current_level_index_m] = 2;
        g_current_level_index_m = space_load_level;
        play_level_loading_slides(lvl, 8, 8, scale_game_frame_count(240), 1);
    } else {
        if (space_load_level == 4 && level_visit_state[4] == 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 9, 10, scale_game_frame_count(240), 0);
            play_level_transition_movie(6);
        }
        if (g_current_level_index_m == LEVEL_UMBRIS && level_visit_state[7] != 2 && level_available_b.f8 != 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 11, 11, scale_game_frame_count(240), 0);
            play_level_transition_movie(7);
        }
        if (space_load_level == 13 && level_visit_state[13] == 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 12, 13, scale_game_frame_count(240), 0);
            play_level_transition_movie(8);
        }
        if (g_current_level_index_m == LEVEL_OLTANIS && level_visit_state[14] != 2 && level_available_b.fF != 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 14, 14, scale_game_frame_count(240), 0);
            play_level_transition_movie(9);
        }
        if (space_load_level == 16 && level_visit_state[16] == 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 15, 16, scale_game_frame_count(240), 0);
            play_level_transition_movie(10);
        }
        if ((unsigned int)g_current_level_index_m < LEVEL_COUNT) {
            ok = 1;
            if (g_current_level_index_m == LEVEL_UMBRIS && level_available_b.f8 == 0) {
                ok = 0;
            }
            if (g_current_level_index_m == LEVEL_OLTANIS && level_available_b.fF == 0) {
                ok = 0;
            }
            if (ok) {
                level_visit_state[g_current_level_index_m] = 2;
            }
        }
        *(short *)&level_archive_load_busy_i = 1;
        done = 0;
        g_current_level_index_m = space_load_level;
        D_0015EF48_m = 0;
        initialize_level_runtime();
        read_file_entry_with_retry(g_current_level_index_m);
        while (space_load_in_progress_m == 0) {
            VU1_sendChain();
            VU1_swapChain();
            PutDrawBufferSmall();
            append_palette_transfer_packet();
            PutDrawBufferLarge();
            UpdatePad();
            update_resident_gameplay_state();
            dispatch_game_state_update();
            memcard_update_state();
            Menu_RunStateHandler();
            VU1_syncChain(1);
            sce_gs_sync_v(0);
            game_frame_counter++;
            tick_space_transition();
            if (!done) {
                done = service_level_archive_load();
            }
        }
        if (!done) {
            do {
                flush_cache(0);
                sce_gs_sync_v(0);
                memcard_update_state();
                Menu_RunStateHandler();
                tick_space_transition();
            } while (service_level_archive_load() == 0);
        }
        while (memory_card_state_b.fDC != 2 || memory_card_state_b.fE4 >= 0) {
            flush_cache(0);
            sce_gs_sync_v(0);
            memcard_update_state();
            Menu_RunStateHandler();
            tick_space_transition();
        }
    }
    sce_cd_sync_s(0);
    cd_read_state_b.fB = 0;
    DMAC_VIF1_Disable();
}
