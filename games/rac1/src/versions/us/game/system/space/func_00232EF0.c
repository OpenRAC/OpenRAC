void play_level_loading_slides(s32 language_index, s32 first_slide, s32 second_slide, s32 duration_ticks,
                               s32 preload_level) {
    u64 shared_texture;
    u64 first_texture;
    u64 second_texture;
    s32 frame;
    s32 alpha;
    s32 fade_in_alpha;
    f32 scroll_offset;
    f32 scroll_start;
    f32 scroll_end;

    prepare_loading_slide_textures(language_index, first_slide, second_slide, &shared_texture, &first_texture,
                                   &second_texture);
    if (preload_level != 0) {
        read_file_entry_with_retry_r((*(s32 *)&g_current_level_index));
        level_archive_load_state_r[0] = 0;
        level_archive_load_busy_r[0] = 0;
    }
    sce_gs_sync_v(0);
    VU1_initChain();
    for (frame = 0; frame < duration_ticks && memory_card_state_r.busy < 3 && memory_card_state_r.state < 0; frame++) {
        alpha = 0x80;
        ResetGsRegisters();
        PutDrawBufferSmall();
        VU1_addGSregister_r2(1, (u64)0x8000 << 16);
        VU1_addGSregister_r2(8, 0);
        g_render_packet_cursor_r.words[0] = 0x30000014;
        fade_in_alpha = frame * 4;
        if (frame <= 0x1F) {
            alpha = fade_in_alpha;
        }
        g_render_packet_cursor_r.words[1] = (s32)second_clear_packet;
        g_render_packet_cursor_r.words[2] = 0;
        g_render_packet_cursor_r.words[3] = 0x50000014;
        g_render_packet_cursor_r.words += 4;
        if (duration_ticks - 0x10 < frame) {
            alpha = (duration_ticks - frame) * 8;
        }
        scroll_offset = IntToFloat(frame % 600) * 0.0016666667f;
        if (first_slide == second_slide) {
            append_scrolling_textured_quad_r(0, g_screen_extent_r2.half_height - 0x20, 0x200, 0x40,
                                             (alpha << 24) | 0x808080, shared_texture, 0.0f, 4.0f, scroll_offset + 0.0f,
                                             scroll_offset + 0.4f);
            DrawTexturedQuad_r(0, g_screen_extent_r2.half_height - 0x20, 0x200, 0x40, 0, 0, 0x200, 0x40, 0x80808080,
                               first_texture);
        } else {
            scroll_start = scroll_offset + 0.0f;
            scroll_end = scroll_offset + 0.4f;
            append_scrolling_textured_quad_r(0, g_screen_extent_r2.half_height - 0x2E, 0x200, 0x40,
                                             (alpha << 24) | 0x808080, shared_texture, 0.0f, 4.0f, scroll_start,
                                             scroll_end);
            DrawTexturedQuad_r(0, g_screen_extent_r2.half_height - 0x2E, 0x200, 0x40, 0, 0, 0x200, 0x40, 0x80808080,
                               first_texture);
            if (frame > 0x40) {
                if (frame < 0x60) {
                    alpha = (frame - 0x40) * 4;
                }
                append_scrolling_textured_quad_r(0, g_screen_extent_r2.half_height, 0x200, 0x40,
                                                 (alpha << 24) | 0x808080, shared_texture, 0.0f, 4.0f, scroll_start,
                                                 scroll_end);
                DrawTexturedQuad_r(0, g_screen_extent_r2.half_height, 0x200, 0x40, 0, 0, 0x200, 0x40, 0x80808080,
                                   second_texture);
            }
        }
        memcard_update_state();
        Menu_RunStateHandler();
        VU1_syncChain(1);
        sce_gs_sync_v(0);
        ResetGsRegistersPr();
        VU1_sendChain();
        VU1_swapChain();
        if (preload_level != 0) {
            if (service_level_archive_load() == 0) {
                if (duration_ticks < frame + 0x14) {
                    duration_ticks = frame + 0x14;
                }
            } else {
                preload_level = 0;
            }
        }
    }
    FadeToBlack(2);
}
